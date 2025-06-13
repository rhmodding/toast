#include "WindowCanvas.hpp"

#include <sstream>
#include <string>

#include <cmath>

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/PlayerManager.hpp"
#include "manager/ThemeManager.hpp"

#include "command/CommandModifyArrangement.hpp"

#include "stb/stb_image_write.h"

#include "texture/APNGHack.hpp"

#include "Macro.hpp"

constexpr float CANVAS_ZOOM_SPEED = .04f;
constexpr float CANVAS_ZOOM_SPEED_FAST = .08f;

static void fitRect(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale) {
    float widthRatio = targetRect.x / rectToFit.x;
    float heightRatio = targetRect.y / rectToFit.y;

    scale = std::min(widthRatio, heightRatio);

    rectToFit.x *= scale;
    rectToFit.y *= scale;
}

static bool pointInPolygon(const ImVec2& point, const ImVec2* vertices, unsigned numVertices) {
    float x = point.x, y = point.y;
    bool inside { false };

    ImVec2 p1 = vertices[0];
    ImVec2 p2;

    for (unsigned i = 1; i <= numVertices; i++) {
        p2 = vertices[i % numVertices];

        // Point is above the minimum y
        // coordinate of the edge
        if (y > std::min(p1.y, p2.y)) {
            // Point is below the maximum y
            // coordinate of the edge
            if (y <= std::max(p1.y, p2.y)) {
                // Point is to the left of the
                // maximum x coordinate of the edge
                if (x <= std::max(p1.x, p2.x)) {
                    // X-intersection of line connecting
                    // point to edge
                    const float xIntersection =
                        (y - p1.y) * (p2.x - p1.x)
                        / (p2.y - p1.y)
                        + p1.x;

                    // Point on same line as edge or
                    // line is on left of x-intersection
                    if (p1.x == p2.x || x <= xIntersection)
                        inside ^= true;
                }
            }
        }

        p1 = p2;
    }

    return inside;
}

constexpr float BOUNDING_INVALID = std::numeric_limits<float>::max();

// Calculate quad bounding of all selected parts
static std::array<ImVec2, 4> calculatePartsBounding(const CellAnimRenderer& renderer, const CellAnim::Arrangement& arrangement, float& quadRotation) {
    std::array<ImVec2, 4> partsBounding ({
        ImVec2(BOUNDING_INVALID, BOUNDING_INVALID),
        ImVec2(BOUNDING_INVALID, BOUNDING_INVALID),
        ImVec2(BOUNDING_INVALID, BOUNDING_INVALID),
        ImVec2(BOUNDING_INVALID, BOUNDING_INVALID)
    });

    quadRotation = 0.f;

    PlayerManager& playerManager = PlayerManager::getInstance();

    const auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    if (!selectionState.anyPartsSelected())
        return partsBounding;

    if (selectionState.singlePartSelected()) {
        unsigned index = selectionState.mSelectedParts[0].index;
        const auto& part = arrangement.parts.at(index);

        if (!part.editorLocked) {
            partsBounding = renderer.getPartWorldQuad(playerManager.getKey().transform, arrangement, index);

            quadRotation = part.transform.angle;
        }

        return partsBounding;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    bool any { false };

    for (const auto& [index, _] : selectionState.mSelectedParts) {
        const auto& part = arrangement.parts.at(index);

        if (part.editorLocked)
            continue;

        any = true;

        std::array<ImVec2, 4> quad = renderer.getPartWorldQuad(playerManager.getKey().transform, arrangement, index);

        for (const auto& vertex : quad) {
            minX = std::min(minX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxX = std::max(maxX, vertex.x);
            maxY = std::max(maxY, vertex.y);
        }
    }

    if (any) {
        partsBounding[0] = { minX, minY };
        partsBounding[1] = { maxX, minY };
        partsBounding[2] = { maxX, maxY };
        partsBounding[3] = { minX, maxY };
    }

    return partsBounding;
}

// Check if selectedParts or arrangement index has changed since last cycle.
static bool selectionChangedSinceLastCycle() {
    PlayerManager& playerManager = PlayerManager::getInstance();

    const auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    const auto& currentSelected = selectionState.mSelectedParts;
    unsigned currentArrangeIdx = playerManager.getArrangementIndex();

    static auto lastSelected { currentSelected };
    static unsigned lastArrangeIdx { currentArrangeIdx };

    bool result = (currentSelected != lastSelected) || (currentArrangeIdx != lastArrangeIdx);

    lastSelected = currentSelected;
    lastArrangeIdx = currentArrangeIdx;

    return result;
}

static float mapRange(float value, float rangeStart, float rangeEnd) {
    return (value - rangeStart) / (rangeEnd - rangeStart);
}

float calcPointsDistance(const ImVec2& a, const ImVec2& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

// In degrees.
static ImVec2 rotateVec2(const ImVec2& v, float angle, const ImVec2& origin) {
    const float s = sinf(angle * ((float)M_PI / 180.f));
    const float c = cosf(angle * ((float)M_PI / 180.f));

    float vx = v.x - origin.x;
    float vy = v.y - origin.y;

    float x = vx * c - vy * s;
    float y = vx * s + vy * c;

    return { x + origin.x, y + origin.y };
}

static ImVec2 getSafeAreaSize() {
    switch (SessionManager::getInstance().getCurrentSession()->type) {
    case CellAnim::CELLANIM_TYPE_RVL:
        return { 832.f, 456.f };
    case CellAnim::CELLANIM_TYPE_CTR:
        return { 400.f, 240.f };
    default:
        return { 0.f, 0.f };
    }
}
static ImVec2 getStereoscopicSafeAreaSize() {
    switch (SessionManager::getInstance().getCurrentSession()->type) {
    case CellAnim::CELLANIM_TYPE_RVL:
        return { 832.f, 456.f }; // Same value.
    case CellAnim::CELLANIM_TYPE_CTR:
        return { 440.f, 240.f };
    default:
        return { 0.f, 0.f };
    }
}

void WindowCanvas::Menubar() {
    const bool isCtr =
        SessionManager::getInstance().getCurrentSession()->type == CellAnim::CELLANIM_TYPE_CTR;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Grid")) {
            if (ImGui::MenuItem("None", nullptr, mState.gridType == CanvasGridType::None))
                mState.gridType = CanvasGridType::None;

            if (ImGui::MenuItem("Dark", nullptr, mState.gridType == CanvasGridType::Dark))
                mState.gridType = CanvasGridType::Dark;

            if (ImGui::MenuItem("Light", nullptr, mState.gridType == CanvasGridType::Light))
                mState.gridType = CanvasGridType::Light;

            if (ImGui::BeginMenu("Custom")) {
                bool enabled = mState.gridType == CanvasGridType::User;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    if (enabled)
                        mState.gridType = CanvasGridType::User;
                    else
                        mState.setDefaultGridType();
                }

                ImGui::SeparatorText("Color Picker");

                ImGui::BeginDisabled(mState.gridType != CanvasGridType::User);
                ImGui::ColorPicker4(
                    "##ColorPicker",
                    &mState.userGridColor.x,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Grid Lines", nullptr, &mState.gridLinesEnable, mState.gridType != CanvasGridType::None);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset", nullptr, false)) {
                mState.resetView();
            }

            if (ImGui::MenuItem("Reset to Safe Area", nullptr, false)) {
                mState.offset = { 0.f, 0.f };

                ImVec2 safeAreaSize = getSafeAreaSize();

                float scale;
                fitRect(safeAreaSize, mCanvasSize, scale);

                mState.zoomFactor = scale - .1f;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Rendering")) {
            if (ImGui::BeginMenu("Draw Bounding Boxes")) {
                ImGui::Checkbox("For all parts", &mState.drawPartBounding);

                ImGui::SeparatorText("Color Picker");

                ImGui::BeginDisabled(!mState.drawPartBounding);
                ImGui::ColorPicker4(
                    "##ColorPicker",
                    &mState.partBoundingDrawColor.x,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Safe Area")) {
                ImGui::Checkbox("Enabled", &mState.safeAreaEnable);

                ImGui::SeparatorText("Options");

                ImGui::BeginDisabled(!mState.safeAreaEnable);
                ImGui::DragScalar("Alpha", ImGuiDataType_U8, &mState.safeAreaAlpha);
                ImGui::EndDisabled();

                ImGui::BeginDisabled(!mState.safeAreaEnable || !isCtr);
                ImGui::Checkbox("Stereoscopic Width", &mState.safeAreaStereoscopic);
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Enable transparent drawing", nullptr, mState.allowTransparentDraw))
                mState.allowTransparentDraw ^= true;

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void WindowCanvas::Update() {
    static bool firstOpen { true };
    if (firstOpen) {
        mState.setDefaultGridType();
        firstOpen = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImGuiIO& io = ImGui::GetIO();

    // Note: ImDrawList uses screen coordinates
    mCanvasTopLeft = ImGui::GetCursorScreenPos();
    mCanvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasBottomRight {
        mCanvasTopLeft.x + mCanvasSize.x,
        mCanvasTopLeft.y + mCanvasSize.y
    };

    Menubar();

    uint32_t backgroundColor;
    switch (mState.gridType) {
    case CanvasGridType::Dark:
        backgroundColor = IM_COL32(50, 50, 50, 255);
        break;
    case CanvasGridType::Light:
        backgroundColor = IM_COL32(255, 255, 255, 255);
        break;
    case CanvasGridType::User:
        backgroundColor = IM_COL32(
            mState.userGridColor.x * 255,
            mState.userGridColor.y * 255,
            mState.userGridColor.z * 255,
            mState.userGridColor.w * 255
        );
        break;
    case CanvasGridType::None:
    default:
        backgroundColor = IM_COL32_BLACK_TRANS;
        break;
    }

    drawList->AddRectFilled(mCanvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions (dragging, hovering, clicking, etc.)
    if (mCanvasSize.x != 0.f && mCanvasSize.y != 0.f)
        ImGui::InvisibleButton("CanvasInteraction", mCanvasSize,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonRight |
            ImGuiButtonFlags_MouseButtonMiddle
        );

    const bool interactionHovered     = ImGui::IsItemHovered();
    const bool interactionActive      = ImGui::IsItemActive();      // Held
    const bool interactionDeactivated = ImGui::IsItemDeactivated(); // Un-held

    const ImVec2 origin(
        mCanvasTopLeft.x + mState.offset.x + static_cast<int>(mCanvasSize.x / 2),
        mCanvasTopLeft.y + mState.offset.y + static_cast<int>(mCanvasSize.y / 2)
    );

    // Dragging
    const float mouseDragThreshold = 1.f;
    const bool draggingWithLMB = ImGui::IsMouseDragging(ImGuiMouseButton_Left, mouseDragThreshold);
    const bool draggingCanvas = interactionActive && (
        draggingWithLMB ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouseDragThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle, mouseDragThreshold)
    );

    static bool panningCanvas { false };

    enum PartsTransformType {
        PartsTransformType_None = 0,
        PartsTransformType_Translate = 1,
        PartsTransformType_Scale,
        PartsTransformType_Rotate
    };

    struct PartsTransformation {
        PartsTransformType type { PartsTransformType_None };
        bool active { false };

        float translateX { 0.f }, translateY { 0.f };
        float scaleX { 1.f }, scaleY { 1.f };
        float rotation { 0.f };

        float pivotX { 0.f }, pivotY { 0.f };

        // Pivot can either be in world-space or local-space (relative to selection center)
        bool pivotIsWorld { false };

        bool canScaleHorizontally { true };
        bool canScaleVertically { true };
    };
    static PartsTransformation partsTransformation;

    if (selectionChangedSinceLastCycle())
        partsTransformation = PartsTransformation {};

    static CellAnim::Arrangement arrangementBeforeMutation;

    static bool movingPivot { false };
    static float pivotBeforeMoveLocal[2] { 0.f, 0.f };
    static float pivotBeforeMoveWorld[2] { 0.f, 0.f };

    PlayerManager& playerManager = PlayerManager::getInstance();

    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    CellAnim::Arrangement& arrangement = playerManager.getArrangement();

    // Select all parts with CTRL+A
    if (ImGui::Shortcut(ImGuiKey_A | ImGuiMod_Ctrl)) {
        selectionState.clearSelectedParts();
        selectionState.mSelectedParts.reserve(arrangement.parts.size());

        for (size_t i = 0; i < arrangement.parts.size(); i++)
            selectionState.setBatchPartSelection(i, true);
    }

    if (panningCanvas) {
        mState.offset.x += io.MouseDelta.x;
        mState.offset.y += io.MouseDelta.y;
    }

    // Canvas zooming
    if (interactionHovered && io.MouseWheel != 0.f) {
        if (io.KeyShift)
            mState.zoomFactor += io.MouseWheel * CANVAS_ZOOM_SPEED_FAST;
        else
            mState.zoomFactor += io.MouseWheel * CANVAS_ZOOM_SPEED;

        mState.clampZoomFactor();
    }

    mCellAnimRenderer.setOffset(origin);
    mCellAnimRenderer.setScale(ImVec2(mState.zoomFactor, mState.zoomFactor));

    float quadRotation { 0.f };
    std::array<ImVec2, 4> partsBounding = calculatePartsBounding(mCellAnimRenderer, arrangement, quadRotation);

    ImVec2 partsBoundingCenter = AVERAGE_IMVEC2(partsBounding[0], partsBounding[2]);
    ImVec2 partsAnmSpaceCenter (
        (partsBoundingCenter.x - origin.x) / mState.zoomFactor,
        (partsBoundingCenter.y - origin.y) / mState.zoomFactor
    );

    const bool noParts = partsBounding[0].x == BOUNDING_INVALID;

    bool hoveringOverParts { false };
    if (interactionHovered && !noParts)
        hoveringOverParts = pointInPolygon(io.MousePos, partsBounding.data(), 4);

    // Top-left, top-right, bottom-right, bottom-left, top, right, bottom, left
    bool hoveringTransformHandles[8] { false };
    // We can't use hoveringOverParts because the handles hang off the edge.
    if (interactionHovered && !noParts) {
        constexpr float hoverRadius = 7.5f;

        // Corners
        for (unsigned i = 0; i < 4; i++) {
            const ImVec2& center = partsBounding[i];

            ImVec2 p1 (
                -hoverRadius + center.x,
                -hoverRadius + center.y
            );
            ImVec2 p3 (
                hoverRadius + center.x,
                hoverRadius + center.y
            );

            hoveringTransformHandles[i] = ImGui::IsMouseHoveringRect(p1, p3);
        }
        // Sides
        for (unsigned i = 0; i < 4; i++) {
            const ImVec2& pointA = partsBounding[i];
            const ImVec2& pointB = partsBounding[(i + 1) % 4];

            const ImVec2 center = AVERAGE_IMVEC2_ROUND(pointA, pointB);

            ImVec2 p1 (
                -hoverRadius + center.x,
                -hoverRadius + center.y
            );
            ImVec2 p3 (
                hoverRadius + center.x,
                hoverRadius + center.y
            );

            hoveringTransformHandles[4 + i] = ImGui::IsMouseHoveringRect(p1, p3);
        }
    }

    ImVec2 pivotPoint;
    if (partsTransformation.pivotIsWorld) {
        pivotPoint = ImVec2(
            partsTransformation.pivotX,
            partsTransformation.pivotY
        );
    }
    else {
        pivotPoint = ImVec2(
            partsAnmSpaceCenter.x + partsTransformation.pivotX,
            partsAnmSpaceCenter.y + partsTransformation.pivotY
        );
    }

    ImVec2 displayPivotPoint = {
        (pivotPoint.x * mState.zoomFactor) + origin.x,
        (pivotPoint.y * mState.zoomFactor) + origin.y
    };
    constexpr float pivotRadius = 6.f;
    constexpr float pivotHoverRadius = 11.f;

    bool hoveringOverPivot = ImGui::IsMouseHoveringRect(
        {
            displayPivotPoint.x - (pivotHoverRadius / 2),
            displayPivotPoint.y - (pivotHoverRadius / 2)
        },
        {
            displayPivotPoint.x + (pivotHoverRadius / 2),
            displayPivotPoint.y + (pivotHoverRadius / 2)
        }
    );

    // Start parts transformation, canvas panning or pivot moving
    if (draggingCanvas && !panningCanvas && !partsTransformation.active && !movingPivot) {
        const bool hoveringAnyTransformHandle =
            hoveringTransformHandles[0] || hoveringTransformHandles[1] ||
            hoveringTransformHandles[2] || hoveringTransformHandles[3] ||
            hoveringTransformHandles[4] || hoveringTransformHandles[5] ||
            hoveringTransformHandles[6] || hoveringTransformHandles[7];

        bool doRotate = io.KeyAlt;

        if (hoveringOverPivot) {
            movingPivot = true;

            pivotBeforeMoveLocal[0] = partsTransformation.pivotX;
            pivotBeforeMoveLocal[1] = partsTransformation.pivotY;
            pivotBeforeMoveWorld[0] = pivotPoint.x;
            pivotBeforeMoveWorld[1] = pivotPoint.y;
        }
        else if (hoveringAnyTransformHandle && !doRotate) {
            pivotBeforeMoveLocal[0] = partsTransformation.pivotX;
            pivotBeforeMoveLocal[1] = partsTransformation.pivotY;
            pivotBeforeMoveWorld[0] = pivotPoint.x;
            pivotBeforeMoveWorld[1] = pivotPoint.y;

            partsTransformation = PartsTransformation {
                .type = PartsTransformType_Scale,
                .active = true,

                .pivotX = pivotPoint.x,
                .pivotY = pivotPoint.y,
                .pivotIsWorld = true,

                .canScaleHorizontally =
                    (!hoveringTransformHandles[4] && !hoveringTransformHandles[6]),
                .canScaleVertically =
                    (!hoveringTransformHandles[5] && !hoveringTransformHandles[7]),
            };

            arrangementBeforeMutation = arrangement;
        }
        else if (hoveringOverParts) {
            pivotBeforeMoveLocal[0] = partsTransformation.pivotX;
            pivotBeforeMoveLocal[1] = partsTransformation.pivotY;
            pivotBeforeMoveWorld[0] = pivotPoint.x;
            pivotBeforeMoveWorld[1] = pivotPoint.y;

            partsTransformation = PartsTransformation {
                .type = doRotate ? PartsTransformType_Rotate : PartsTransformType_Translate,
                .active = true,

                .pivotX = pivotPoint.x,
                .pivotY = pivotPoint.y,
                .pivotIsWorld = true
            };

            arrangementBeforeMutation = arrangement;
        }
        else {
            if (!ConfigManager::getInstance().getConfig().canvasLMBPanEnabled) {
                if (!draggingWithLMB)
                    panningCanvas = true;
            }
            else
                panningCanvas = true;
        }
    }

    if (movingPivot) {
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);

        partsTransformation.pivotX =
            pivotBeforeMoveLocal[0] + (dragDelta.x / mState.zoomFactor);
        partsTransformation.pivotY =
            pivotBeforeMoveLocal[1] + (dragDelta.y / mState.zoomFactor);
    }

    auto myPartsBounding = partsBounding;

    // Temporarily apply partsTransformation (reset on end of update)
    if (partsTransformation.active) {
        const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);
        const ImVec2 mouseDragStart (
            ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Left].x - origin.x,
            ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Left].y - origin.y
        );
        const ImVec2 mousePosition (
            ImGui::GetIO().MousePos.x - origin.x,
            ImGui::GetIO().MousePos.y - origin.y
        );

        const bool shiftHeld = ImGui::GetIO().KeyShift;

        switch (partsTransformation.type) {
        case PartsTransformType_Translate: {
            ImVec2 scaledOffset (
                dragDelta.x / mState.zoomFactor,
                dragDelta.y / mState.zoomFactor
            );

            const auto& key = playerManager.getKey();

            float theta = -key.transform.angle * (float)M_PI / 180.f;

            float rotatedX = scaledOffset.x * cosf(theta) - scaledOffset.y * sinf(theta);
            float rotatedY = scaledOffset.x * sinf(theta) + scaledOffset.y * cosf(theta);

            float offsX = rotatedX / key.transform.scale.x;
            float offsY = rotatedY / key.transform.scale.y;

            partsTransformation.translateX = offsX;
            partsTransformation.translateY = offsY;

            // In this case the pivot moves with the selection.
            // This is just visuals; the previous pivot is restored when the transform is finalized
            partsTransformation.pivotX = pivotBeforeMoveWorld[0] + int(scaledOffset.x);
            partsTransformation.pivotY = pivotBeforeMoveWorld[1] + int(scaledOffset.y);
        } break;
        case PartsTransformType_Scale: {
            // TODO: PRIORITY scaling not working as expected with any type of rotation

            const auto& keyTransform = playerManager.getKey().transform;

            float keyCompensateRot = -keyTransform.angle;

            float partCompensateRot = 0.f;
            ImVec2 partAvgCenter ( 0.f, 0.f );
            for (const auto& [index, _] : selectionState.mSelectedParts) {
                const auto& part = arrangement.parts.at(index);

                partCompensateRot -= part.transform.angle;

                float centerX = part.transform.position.x + (part.regionSize.x * part.transform.scale.x * .5f);
                float centerY = part.transform.position.y + (part.regionSize.y * part.transform.scale.y * .5f);

                partAvgCenter.x += centerX;
                partAvgCenter.y += centerY;
            }
            partCompensateRot /= selectionState.mSelectedParts.size();

            partAvgCenter.x = (partAvgCenter.x / selectionState.mSelectedParts.size()) * keyTransform.scale.x * mState.zoomFactor;
            partAvgCenter.y = (partAvgCenter.y / selectionState.mSelectedParts.size()) * keyTransform.scale.y * mState.zoomFactor;

            ImVec2 partOrigin = { origin.x + partAvgCenter.x, origin.y + partAvgCenter.y };

            myPartsBounding = partsBounding;
            for (ImVec2& point : myPartsBounding) {
                point = rotateVec2(point, keyCompensateRot, origin);
            }

            // const uint32_t colorLine = IM_COL32_WHITE;

            // drawList->AddQuad(myPartsBounding[0], myPartsBounding[1], myPartsBounding[2], myPartsBounding[3], colorLine, 1.5f);

            for (ImVec2& point : myPartsBounding) {
                point = rotateVec2(point, partCompensateRot, partOrigin);
            }

            // drawList->AddCircleFilled(partOrigin, 2.f, colorLine);

            // drawList->AddQuad(myPartsBounding[0], myPartsBounding[1], myPartsBounding[2], myPartsBounding[3], colorLine, 2.f);

            ImVec2 myMouseDragStart = rotateVec2(mouseDragStart, keyCompensateRot, { 0.f, 0.f });
            myMouseDragStart = rotateVec2(myMouseDragStart, partCompensateRot, partAvgCenter);
            ImVec2 myMousePosition = rotateVec2(mousePosition, keyCompensateRot, { 0.f, 0.f });
            myMousePosition = rotateVec2(myMousePosition, partCompensateRot, partAvgCenter);

            // drawList->AddLine({myMousePosition.x+origin.x, myMousePosition.y+origin.y}, {myMouseDragStart.x+origin.x,myMouseDragStart.y+origin.y}, colorLine, 2.f);

            ImVec2 myDragDelta = {
                myMousePosition.x - myMouseDragStart.x,
                myMousePosition.y - myMouseDragStart.y
            };

            float startWidth = myPartsBounding[2].x - myPartsBounding[0].x;
            float startHeight = myPartsBounding[2].y - myPartsBounding[0].y;

            ImVec2 myPivot = rotateVec2({
                (partsTransformation.pivotX * mState.zoomFactor),
                (partsTransformation.pivotY * mState.zoomFactor)
            }, keyCompensateRot, { 0.f, 0.f });

            const float rangeStartX = myPartsBounding[0].x - origin.x;
            const float rangeEndX = myPartsBounding[1].x - origin.x;
            const float rangeStartY = myPartsBounding[0].y - origin.y;
            const float rangeEndY = myPartsBounding[3].y - origin.y;

            const float rangeMapX = mapRange(myMouseDragStart.x, rangeStartX, rangeEndX);
            const float rangeMapY = mapRange(myMouseDragStart.y, rangeStartY, rangeEndY);

            float quadX = rangeMapX * startWidth + myPartsBounding[0].x;
            float quadY = rangeMapY * startHeight + myPartsBounding[0].y;

            const float inputEndX = quadX - origin.x;
            const float inputEndY = quadY - origin.y;

            const float mapX = mapRange(myPivot.x - myDragDelta.x, myPivot.x, inputEndX);
            const float mapY = mapRange(myPivot.y - myDragDelta.y, myPivot.y, inputEndY);

            float newWidth = startWidth * (1.f - mapX);
            float newHeight = startHeight * (1.f - mapY);

            float scaleX = (newWidth / startWidth);
            float scaleY = (newHeight / startHeight);

            if (shiftHeld) {
                float scale = AVERAGE_FLOATS(scaleX, scaleY);
                partsTransformation.scaleX = scale;
                partsTransformation.scaleY = scale;
            }
            else {
                partsTransformation.scaleX = scaleX;
                partsTransformation.scaleY = scaleY;
            }
        } break;
        case PartsTransformType_Rotate: {
            float pivotRealX = (partsTransformation.pivotX * mState.zoomFactor);
            float pivotRealY = (partsTransformation.pivotY * mState.zoomFactor);

            float startAngle = std::atan2(
                mouseDragStart.y - pivotRealY, // y
                mouseDragStart.x - pivotRealX // x
            );
            float nowAngle = std::atan2(
                mousePosition.y - pivotRealY, // y
                mousePosition.x - pivotRealX // x
            );

            partsTransformation.rotation = (nowAngle - startAngle) * (180.f / (float)M_PI);

            // Snap to 45deg if Shift is held.
            if (shiftHeld)
                partsTransformation.rotation = roundf(partsTransformation.rotation / 45.f) * 45.f;
        } break;

        default:
            break;
        }

        // Apply transformation
        for (const auto& [index, _] : selectionState.mSelectedParts) {
            auto& part = arrangement.parts.at(index);

            if (part.editorLocked)
                continue;

            if (!partsTransformation.canScaleHorizontally)
                partsTransformation.scaleX = 1.f;
            if (!partsTransformation.canScaleVertically)
                partsTransformation.scaleY = 1.f;

            const auto& keyTransform = playerManager.getKey().transform;

            float keyScaleX = keyTransform.scale.x;
            float keyScaleY = keyTransform.scale.y;
            float keyPosX = keyTransform.position.x;
            float keyPosY = keyTransform.position.y;

            // Transform to part-space

            ImVec2 pivotPS = rotateVec2(
                { partsTransformation.pivotX, partsTransformation.pivotY },
                -keyTransform.angle, { 0.f, 0.f }
            );
            pivotPS.x = (pivotPS.x - keyPosX) / keyScaleX;
            pivotPS.y = (pivotPS.y - keyPosY) / keyScaleY;

            float posAddedX = part.transform.position.x + partsTransformation.translateX;
            float posAddedY = part.transform.position.y + partsTransformation.translateY;

            float offsetX = (posAddedX - pivotPS.x) * (1.f - partsTransformation.scaleX);
            float offsetY = (posAddedY - pivotPS.y) * (1.f - partsTransformation.scaleY);

            part.transform.position.x = posAddedX - offsetX;
            part.transform.position.y = posAddedY - offsetY;

            part.transform.scale.x *= partsTransformation.scaleX;
            part.transform.scale.y *= partsTransformation.scaleY;

            float pX = pivotPS.x - (part.regionSize.x * part.transform.scale.x) / 2.f;
            float pY = pivotPS.y - (part.regionSize.y * part.transform.scale.y) / 2.f;

            // Compensation on position for transformation rotation.

            float dx = part.transform.position.x - pX;
            float dy = part.transform.position.y - pY;

            float cosAngle = cosf(partsTransformation.rotation * ((float)M_PI / 180.f));
            float sinAngle = sinf(partsTransformation.rotation * ((float)M_PI / 180.f));

            float rotatedX = dx * cosAngle - dy * sinAngle;
            float rotatedY = dx * sinAngle + dy * cosAngle;

            part.transform.position.x = rotatedX + pX;
            part.transform.position.y = rotatedY + pY;

            part.transform.angle += partsTransformation.rotation;
        }

        partsBounding = calculatePartsBounding(mCellAnimRenderer, arrangement, quadRotation);
        partsBoundingCenter = AVERAGE_IMVEC2(partsBounding[0], partsBounding[2]);
        partsAnmSpaceCenter = ImVec2(
            (partsBoundingCenter.x - origin.x) / mState.zoomFactor,
            (partsBoundingCenter.y - origin.y) / mState.zoomFactor
        );
    }

    // Update pivotPoint since it might have updated
    if (partsTransformation.pivotIsWorld) {
        pivotPoint = ImVec2(
            partsTransformation.pivotX,
            partsTransformation.pivotY
        );
    }
    else {
        pivotPoint = ImVec2(
            partsAnmSpaceCenter.x + partsTransformation.pivotX,
            partsAnmSpaceCenter.y + partsTransformation.pivotY
        );
    }

    // Stop & apply new parts, stop panning or deselect all parts
    if (interactionDeactivated) {
        if (panningCanvas)
            panningCanvas = false;
        else if (movingPivot)
            movingPivot = false;
        else if (partsTransformation.active) {
            float newPivotX, newPivotY;
            if (partsTransformation.type == PartsTransformType_Translate) {
                newPivotX = pivotBeforeMoveLocal[0];
                newPivotY = pivotBeforeMoveLocal[1];
            }
            else {
                newPivotX = FLOOR_FLOAT(pivotPoint.x - partsAnmSpaceCenter.x);
                newPivotY = FLOOR_FLOAT(pivotPoint.y - partsAnmSpaceCenter.y);
            }

            partsTransformation = PartsTransformation {
                .type = PartsTransformType_None,
                .active = false,
                .pivotX = newPivotX,
                .pivotY = newPivotY,
                .pivotIsWorld = false
            };

            pivotPoint = ImVec2(
                partsAnmSpaceCenter.x + newPivotX,
                partsAnmSpaceCenter.y + newPivotY
            );

            SessionManager& sessionManager = SessionManager::getInstance();

            // Reverse the role of arrangementBeforeMutation for the command submit
            std::swap(arrangementBeforeMutation, arrangement);

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangement>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getArrangementIndex(),
                arrangementBeforeMutation // Used as the new arrangement
            ));
        }
        else
            selectionState.clearSelectedParts();
    }

    // All drawing operations
    {
        bool isBackgroundLight = false;
        switch (mState.gridType) {
        case CanvasGridType::Dark:
            isBackgroundLight = false;
            break;
        case CanvasGridType::Light:
            isBackgroundLight = true;
            break;
        case CanvasGridType::User:
            isBackgroundLight = mState.getUserGridColorLumi() > .5f;
            break;
        case CanvasGridType::None:
        default:
            isBackgroundLight = ThemeManager::getInstance().getThemeIsLight();
            break;
        }

        drawList->PushClipRect(mCanvasTopLeft, canvasBottomRight, true);
        {
            // Draw grid
            if (mState.gridLinesEnable && mState.gridType != CanvasGridType::None) {
                const float GRID_STEP = 64.f * mState.zoomFactor;

                constexpr uint32_t normalColor = IM_COL32(200, 200, 200, 40);
                constexpr uint32_t centerColorX = IM_COL32(255, 0, 0, 70);
                constexpr uint32_t centerColorY = IM_COL32(0, 255, 0, 70);

                for (
                    float x = fmodf(mState.offset.x + static_cast<int>(mCanvasSize.x / 2), GRID_STEP);
                    x < mCanvasSize.x;
                    x += GRID_STEP
                ) {
                    bool centered = fabs((mCanvasTopLeft.x + x) - origin.x) < .01f;
                    drawList->AddLine(
                        { mCanvasTopLeft.x + x, mCanvasTopLeft.y },
                        { mCanvasTopLeft.x + x, canvasBottomRight.y },
                        centered ? centerColorX : normalColor
                    );
                }

                for (
                    float y = fmodf(mState.offset.y + static_cast<int>(mCanvasSize.y / 2), GRID_STEP);
                    y < mCanvasSize.y;
                    y += GRID_STEP
                ) {
                    bool centered = fabs((mCanvasTopLeft.y + y) - origin.y) < .01f;
                    drawList->AddLine(
                        { mCanvasTopLeft.x, mCanvasTopLeft.y + y },
                        { canvasBottomRight.x, mCanvasTopLeft.y + y },
                        centered ? centerColorY : normalColor
                    );
                }
            }

            // Draw cellanim.
            {
                const auto& currentSession = SessionManager::getInstance().getCurrentSession();

                mCellAnimRenderer.linkCellAnim(currentSession->getCurrentCellAnim().object);
                mCellAnimRenderer.linkTextureGroup(currentSession->sheets);

                const auto& onionSkinState = playerManager.getOnionSkinState();

                bool drawOnionSkin = onionSkinState.enabled;
                bool drawUnder = onionSkinState.drawUnder;

                if (drawOnionSkin && drawUnder) {
                    mCellAnimRenderer.DrawOnionSkin(
                        drawList,
                        playerManager.getAnimation(), playerManager.getKeyIndex(),
                        onionSkinState.backCount,
                        onionSkinState.frontCount,
                        onionSkinState.rollOver,
                        onionSkinState.opacity
                    );
                }

                mCellAnimRenderer.Draw(
                    drawList,
                    playerManager.getAnimation(), playerManager.getKeyIndex(),
                    mState.allowTransparentDraw
                );

                if (drawOnionSkin && !drawUnder) {
                    mCellAnimRenderer.DrawOnionSkin(
                        drawList,
                        playerManager.getAnimation(), playerManager.getKeyIndex(),
                        onionSkinState.backCount,
                        onionSkinState.frontCount,
                        onionSkinState.rollOver,
                        onionSkinState.opacity
                    );
                }
            }

            // Draw safe area if enabled
            if (mState.safeAreaEnable) {
                const ImVec2 safeArea = mState.safeAreaStereoscopic ?
                    getStereoscopicSafeAreaSize() : getSafeAreaSize();

                ImVec2 boxTopLeft (
                    origin.x - ((safeArea.x * mState.zoomFactor) / 2.f),
                    origin.y - ((safeArea.y * mState.zoomFactor) / 2.f)
                );
                ImVec2 boxBottomRight (
                    boxTopLeft.x + (safeArea.x * mState.zoomFactor),
                    boxTopLeft.y + (safeArea.y * mState.zoomFactor)
                );

                uint32_t color = IM_COL32(0, 0, 0, mState.safeAreaAlpha);

                drawList->AddRectFilled( // Top Box
                    mCanvasTopLeft,
                    { canvasBottomRight.x, boxTopLeft.y },
                    color
                );
                drawList->AddRectFilled( // Left Box
                    { mCanvasTopLeft.x, boxTopLeft.y },
                    { boxTopLeft.x, boxBottomRight.y },
                    color
                );
                drawList->AddRectFilled( // Right Box
                    { boxBottomRight.x, boxTopLeft.y },
                    { canvasBottomRight.x, boxBottomRight.y },
                    color
                );
                drawList->AddRectFilled( // Bottom Box
                    { mCanvasTopLeft.x, boxBottomRight.y },
                    canvasBottomRight,
                    color
                );
            }
        }
        drawList->PopClipRect();

        // Draw selection box + handles & pivot point.
        if (!noParts /* && appState.mFocusOnSelectedPart */) { // TODO
            const uint32_t colorLine = isBackgroundLight ? IM_COL32(0,0,0,0xFF) : IM_COL32(0xFF,0xFF,0xFF,0xFF);
            const uint32_t colorFill = isBackgroundLight ? IM_COL32(0xFF,0xFF,0xFF,0xFF) : IM_COL32(0,0,0,0xFF);

            constexpr float lineThickness = 1.5f;
            constexpr float handleLineThickness = 2.f;

            constexpr float handleRadius = 4.5f;

            drawList->AddQuad(partsBounding[0], partsBounding[1], partsBounding[2], partsBounding[3], colorLine, lineThickness);

            // Corners
            for (unsigned i = 0; i < 4; i++) {
                const ImVec2& center = partsBounding[i];

                ImVec2 p1 (
                    -handleRadius + center.x,
                    -handleRadius + center.y
                );
                ImVec2 p3 (
                    handleRadius + center.x,
                    handleRadius + center.y
                );

                drawList->AddRectFilled(p1, p3, colorFill, 0.f, ImDrawFlags_None);
                drawList->AddRect(p1, p3, colorLine, 0.f, ImDrawFlags_None, handleLineThickness);
            }

            // Sides
            for (unsigned i = 0; i < 4; i++) {
                const ImVec2& pointA = partsBounding[i];
                const ImVec2& pointB = partsBounding[(i + 1) % 4];

                const ImVec2 center = AVERAGE_IMVEC2_ROUND(pointA, pointB);

                ImVec2 p1 (
                    -handleRadius + center.x,
                    -handleRadius + center.y
                );
                /*
                ImVec2 p2 (
                    radius  + center.x,
                    -radius + center.y
                );
                */
                ImVec2 p3 (
                    handleRadius + center.x,
                    handleRadius + center.y
                );
                /*
                ImVec2 p4 (
                    -radius + center.x,
                    radius  + center.y
                );
                */

                drawList->AddRectFilled(p1, p3, colorFill, 0.f, ImDrawFlags_None);
                drawList->AddRect(p1, p3, colorLine, 0.f, ImDrawFlags_None, handleLineThickness);
            }

            // Update displayPivotPoint since pivotPoint might have updated
            displayPivotPoint = {
                (pivotPoint.x * mState.zoomFactor) + origin.x,
                (pivotPoint.y * mState.zoomFactor) + origin.y
            };

            // Draw the pivot
            const bool pivotDrawActive = hoveringOverPivot || movingPivot;

            float pivotDrawRadius = pivotDrawActive ? pivotRadius + 1.f : pivotRadius;
            constexpr float pivotLineThickness = 2.25f;

            constexpr float epsilon = 1e-2f;

            if ((fabs(partsTransformation.pivotX) > epsilon || partsTransformation.pivotY > epsilon) && !partsTransformation.pivotIsWorld) {
                constexpr uint32_t pivotColorOld = IM_COL32(57, 57, 57, .3f * 255);
                ImVec2 displayPivotPointOld = {
                    (partsAnmSpaceCenter.x * mState.zoomFactor) + origin.x,
                    (partsAnmSpaceCenter.y * mState.zoomFactor) + origin.y
                };

                constexpr float pivotRadiusOld = pivotRadius - 1.f;

                drawList->AddCircleFilled(displayPivotPointOld, pivotRadiusOld, pivotColorOld);
                drawList->AddCircle(displayPivotPointOld, pivotRadiusOld, pivotColorOld, 0, 3.f);
            }

            drawList->AddCircleFilled(displayPivotPoint, pivotDrawRadius, IM_COL32_WHITE);
            drawList->AddCircle(displayPivotPoint, pivotDrawRadius, IM_COL32_BLACK, 0, pivotLineThickness);
        }

        DrawCanvasText();
    }

    if (partsTransformation.active)
        arrangement = arrangementBeforeMutation;

    ImGui::End();
}

void WindowCanvas::DrawCanvasText() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    uint32_t textColor { IM_COL32_BLACK };
    switch (mState.gridType) {
    case CanvasGridType::Dark:
        textColor = IM_COL32_WHITE;
        break;
    case CanvasGridType::Light:
        textColor = IM_COL32_BLACK;
        break;
    case CanvasGridType::User:
        textColor = (mState.getUserGridColorLumi() > .5f) ? IM_COL32_BLACK : IM_COL32_WHITE;
        break;
    case CanvasGridType::None:
    default:
        textColor = ThemeManager::getInstance().getThemeIsLight() ? IM_COL32_BLACK : IM_COL32_WHITE;
        break;
    }

    const float textDrawLeft = mCanvasTopLeft.x + 10.f;
    float textDrawTop = mCanvasTopLeft.y + 5.f;

    constexpr float textDrawGap = 3.f;

    if ((mState.offset.x != 0.f) || (mState.offset.y != 0.f)) {
        std::string str;
        {
            std::ostringstream fmtStream;
            fmtStream <<
                "Offset: " <<
                std::to_string(mState.offset.x) << ", " <<
                std::to_string(mState.offset.y);

            str = fmtStream.str();
        }

        drawList->AddText(
            { textDrawLeft, textDrawTop },
            textColor, str.c_str()
        );

        textDrawTop += textDrawGap + ImGui::CalcTextSize(str.c_str()).y;
    }

    if (mState.zoomFactor != 1.f) {
        std::string str;
        {
            std::ostringstream fmtStream;
            fmtStream <<
                "Zoom: " <<
                std::to_string(static_cast<int>(mState.zoomFactor * 100.f)) <<
                "% (hold [Shift] to zoom faster)";

            str = fmtStream.str();
        }

        drawList->AddText(
            { textDrawLeft, textDrawTop },
            textColor, str.c_str()
        );

        textDrawTop += textDrawGap + ImGui::CalcTextSize(str.c_str()).y;
    }
}
