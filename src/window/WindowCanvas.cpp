#include "WindowCanvas.hpp"

#include <sstream>
#include <string>

#include <cmath>

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

#include "../command/CommandModifyArrangement.hpp"
#include "../command/CommandModifyArrangementPart.hpp"

#include "../common.hpp"

constexpr float CANVAS_ZOOM_SPEED = .04f;
constexpr float CANVAS_ZOOM_SPEED_FAST = .08f;

static void fitRect(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale) {
    float widthRatio = targetRect.x / rectToFit.x;
    float heightRatio = targetRect.y / rectToFit.y;

    scale = std::min(widthRatio, heightRatio);

    rectToFit.x *= scale;
    rectToFit.y *= scale;
}

// Check if point is inside a polygon defined by vertices
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

// Calculate quad bounding of all selected parts
static std::array<ImVec2, 4> calculatePartsBounding(const RvlCellAnim::Arrangement& arrangement, float& quadRotation) {
    std::array<ImVec2, 4> partsBounding ({{ -FLT_MAX, -FLT_MAX }});

    FLT_EPSILON;
    
    quadRotation = 0.f;

    AppState& appState = AppState::getInstance();

    if (!appState.anyPartsSelected())
        return partsBounding;

    if (appState.singlePartSelected()) {
        unsigned index = appState.selectedParts[0].index;
        const auto& part = arrangement.parts.at(index);

        if (!part.editorLocked) {
            partsBounding = appState.globalAnimatable.getPartWorldQuad(
                appState.globalAnimatable.getCurrentKey(), index
            );

            quadRotation = part.transform.angle;
        }

        return partsBounding;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    bool any { false };

    for (const auto& [index, _] : appState.selectedParts) {
        const auto& part = arrangement.parts.at(index);

        if (part.editorLocked)
            continue;
        
        any = true;

        std::array<ImVec2, 4> quad = appState.globalAnimatable.getPartWorldQuad(
            appState.globalAnimatable.getCurrentKey(), index
        );
        
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
    AppState& appState = AppState::getInstance();

    const auto& currentSelected = appState.selectedParts;
    const auto& currentArrangeIdx = appState.globalAnimatable.getCurrentKey()->arrangementIndex;

    static auto lastSelected { currentSelected };
    static auto lastArrangeIdx { currentArrangeIdx };

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

// Screen safe area for Fever (RVL)
constexpr float RVL_SAFE_X = 832;
constexpr float RVL_SAFE_Y = 456;

// Screen safe area for Megamix (CTR)
constexpr float CTR_SAFE_X = 440;
constexpr float CTR_SAFE_Y = 240;

void WindowCanvas::Menubar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Grid")) {
            if (ImGui::MenuItem("None", nullptr, gridType == GridType_None))
                gridType = GridType_None;

            if (ImGui::MenuItem("Dark", nullptr, gridType == GridType_Dark))
                gridType = GridType_Dark;

            if (ImGui::MenuItem("Light", nullptr, gridType == GridType_Light))
                gridType = GridType_Light;

            if (ImGui::BeginMenu("Custom")) {
                bool enabled = gridType == GridType_Custom;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    if (enabled)
                        gridType = GridType_Custom;
                    else
                        gridType = AppState::getInstance().getDarkThemeEnabled() ? GridType_Dark : GridType_Light;
                };

                ImGui::SeparatorText("Color Picker");

                ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&customGridColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Grid Lines", nullptr, &this->enableGridLines, gridType != GridType_None);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset", nullptr, false)) {
                this->canvasOffset = { 0.f, 0.f };
                this->canvasZoom = 1.f;
            }

            if (ImGui::MenuItem("Reset to Safe Area", nullptr, false)) {
                this->canvasOffset = { 0.f, 0.f };

                ImVec2 rect = { RVL_SAFE_X, RVL_SAFE_Y };

                float scale;
                fitRect(rect, this->canvasSize, scale);

                this->canvasZoom = scale - .1f;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Rendering")) {
            if (ImGui::BeginMenu("Draw Bounding Boxes")) {
                ImGui::Checkbox("For all parts", &drawAllBounding);

                ImGui::SeparatorText("Color Picker");

                ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&partBoundingDrawColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Safe Area")) {
                ImGui::Checkbox("Enabled", &this->showSafeArea);

                ImGui::SeparatorText("Options");

                ImGui::BeginDisabled(!this->showSafeArea);
                ImGui::DragScalar("Alpha", ImGuiDataType_U8, &this->safeAreaAlpha);
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Enable part transparency", nullptr, allowOpacity))
                this->allowOpacity ^= true;

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void WindowCanvas::Update() {
    static bool firstOpen { true };
    if (firstOpen) {
        this->gridType = AppState::getInstance().getDarkThemeEnabled() ?
            GridType_Dark : GridType_Light;

        firstOpen = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImGuiIO& io = ImGui::GetIO();

    // Note: ImDrawList uses screen coordinates
    this->canvasTopLeft = ImGui::GetCursorScreenPos();
    this->canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasBottomRight {
        this->canvasTopLeft.x + this->canvasSize.x,
        this->canvasTopLeft.y + this->canvasSize.y
    };

    this->Menubar();

    uint32_t backgroundColor; // Black
    switch (this->gridType) {
    case GridType_None:
        backgroundColor = 0x00000000; // Transparent Black
        break;

    case GridType_Dark:
        backgroundColor = IM_COL32(50, 50, 50, 255);
        break;

    case GridType_Light:
        backgroundColor = IM_COL32(255, 255, 255, 255);
        break;

    case GridType_Custom:
        backgroundColor = IM_COL32(
            customGridColor.x * 255,
            customGridColor.y * 255,
            customGridColor.z * 255,
            customGridColor.w * 255
        );
        break;

    default:
        break;
    }

    drawList->AddRectFilled(this->canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions (dragging, hovering, clicking, etc.)
    ImGui::InvisibleButton("CanvasInteraction", this->canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle
    );

    const bool interactionHovered     = ImGui::IsItemHovered();
    const bool interactionActive      = ImGui::IsItemActive();      // Held
    const bool interactionDeactivated = ImGui::IsItemDeactivated(); // Un-held

    const ImVec2 origin(
        this->canvasTopLeft.x + this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2),
        this->canvasTopLeft.y + this->canvasOffset.y + static_cast<int>(this->canvasSize.y / 2)
    );

    // Dragging
    const float mouseDragThreshold = 1.f;
    const bool draggingCanvas = interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouseDragThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle, mouseDragThreshold) ||
        (
            ConfigManager::getInstance().getConfig().canvasLMBPanEnabled &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, mouseDragThreshold)
        )
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

    static RvlCellAnim::Arrangement arrangementBeforeMutation;

    static bool movingPivot { false };
    static float pivotBeforeMoveLocal[2] { 0.f, 0.f };
    static float pivotBeforeMoveWorld[2] { 0.f, 0.f };

    Animatable& globalAnimatable = AppState::getInstance().globalAnimatable;
    AppState& appState = AppState::getInstance();

    RvlCellAnim::Arrangement& arrangement = *globalAnimatable.getCurrentArrangement();

    // Select all parts with CTRL+A
    if (ImGui::Shortcut(ImGuiKey_A | ImGuiMod_Ctrl)) {
        appState.clearSelectedParts();
        appState.selectedParts.reserve(arrangement.parts.size());

        for (unsigned i = 0; i < arrangement.parts.size(); i++, appState.spSelectionOrder++) {
            appState.setBatchPartSelection(i, true);
        }
    }

    if (panningCanvas) {
        this->canvasOffset.x += io.MouseDelta.x;
        this->canvasOffset.y += io.MouseDelta.y;
    }

    // Canvas zooming
    {
        const float maxZoom = 10.f;
        const float minZoom = .1f;

        if (interactionHovered) {
            if (io.MouseWheel != 0) {
                if (io.KeyShift)
                    this->canvasZoom += io.MouseWheel * CANVAS_ZOOM_SPEED_FAST;
                else
                    this->canvasZoom += io.MouseWheel * CANVAS_ZOOM_SPEED;
            }

            if (this->canvasZoom < minZoom)
                this->canvasZoom = minZoom;

            if (this->canvasZoom > maxZoom)
                this->canvasZoom = maxZoom;
        }
    }

    globalAnimatable.offset = origin;

    globalAnimatable.scaleX = this->canvasZoom;
    globalAnimatable.scaleY = this->canvasZoom;

    float quadRotation { 0.f };
    std::array<ImVec2, 4> partsBounding = calculatePartsBounding(arrangement, quadRotation);

    ImVec2 partsBoundingCenter = AVERAGE_IMVEC2(partsBounding[0], partsBounding[2]);
    ImVec2 partsAnmSpaceCenter (
        (partsBoundingCenter.x - origin.x) / this->canvasZoom,
        (partsBoundingCenter.y - origin.y) / this->canvasZoom
    );

    const bool noParts = partsBounding[0].x == -FLT_MAX;

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
        (pivotPoint.x * this->canvasZoom) + origin.x,
        (pivotPoint.y * this->canvasZoom) + origin.y
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
        else
            panningCanvas = true;
    }

    if (movingPivot) {
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);

        partsTransformation.pivotX =
            pivotBeforeMoveLocal[0] + (dragDelta.x / this->canvasZoom);
        partsTransformation.pivotY =
            pivotBeforeMoveLocal[1] + (dragDelta.y / this->canvasZoom);
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
                dragDelta.x / this->canvasZoom,
                dragDelta.y / this->canvasZoom
            );

            float theta = -globalAnimatable.getCurrentKey()->transform.angle * M_PI / 180.f;

            float rotatedX = scaledOffset.x * cosf(theta) - scaledOffset.y * sinf(theta);
            float rotatedY = scaledOffset.x * sinf(theta) + scaledOffset.y * cosf(theta);

            float offsX = rotatedX / globalAnimatable.getCurrentKey()->transform.scaleX;
            float offsY = rotatedY / globalAnimatable.getCurrentKey()->transform.scaleY;

            partsTransformation.translateX = offsX;
            partsTransformation.translateY = offsY;

            // In this case the pivot moves with the selection.
            // This is just visuals; the previous pivot is restored when the transform is finalized
            partsTransformation.pivotX = pivotBeforeMoveWorld[0] + int(scaledOffset.x);
            partsTransformation.pivotY = pivotBeforeMoveWorld[1] + int(scaledOffset.y);
        } break;
        case PartsTransformType_Scale: {
            // TODO: PRIORITY scaling not working as expected with any type of rotation

            const auto& keyTransform = globalAnimatable.getCurrentKey()->transform;

            /*
                quad
                0,  1,
                3,  2
            */

            float compensateRot = -keyTransform.angle;

            printf("compensateRot:%f\n", compensateRot);

            myPartsBounding = partsBounding;
            for (auto& point : myPartsBounding)
                point = rotateVec2(point, compensateRot, origin);

            auto myMouseDragStart = rotateVec2(mouseDragStart, compensateRot, { 0.f, 0.f });

            auto myDragDelta = rotateVec2(dragDelta, compensateRot, { 0.f, 0.f });

            float startWidth = myPartsBounding[2].x - myPartsBounding[0].x;
            float startHeight = myPartsBounding[2].y - myPartsBounding[0].y;

            float pivotRealX = (partsTransformation.pivotX * this->canvasZoom);
            float pivotRealY = (partsTransformation.pivotY * this->canvasZoom);

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

            const float mapX = mapRange(pivotRealX - myDragDelta.x, pivotRealX, inputEndX);
            const float mapY = mapRange(pivotRealY - myDragDelta.y, pivotRealY, inputEndY);

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
            float pivotRealX = (partsTransformation.pivotX * this->canvasZoom);
            float pivotRealY = (partsTransformation.pivotY * this->canvasZoom);

            float startAngle = std::atan2(
                mouseDragStart.y - pivotRealY, // y
                mouseDragStart.x - pivotRealX // x
            );
            float nowAngle = std::atan2(
                mousePosition.y - pivotRealY, // y
                mousePosition.x - pivotRealX // x
            );

            partsTransformation.rotation = (nowAngle - startAngle) * (180.f / M_PI);

            // Snap to 45deg if Shift is held.
            if (shiftHeld) {
                partsTransformation.rotation =
                    roundf(partsTransformation.rotation / 45.f) * 45.f;
            }
        } break;
        
        default:
            break;
        }

        for (const auto& [index, _] : appState.selectedParts) {
            auto& part = arrangement.parts.at(index);

            if (part.editorLocked)
                continue;

            if (!partsTransformation.canScaleHorizontally)
                partsTransformation.scaleX = 1.f;
            if (!partsTransformation.canScaleVertically)
                partsTransformation.scaleY = 1.f;

            // Obtain key transformation values
            float keyScaleX = globalAnimatable.getCurrentKey()->transform.scaleX;
            float keyScaleY = globalAnimatable.getCurrentKey()->transform.scaleY;
            float keyPosX = globalAnimatable.getCurrentKey()->transform.positionX;
            float keyPosY = globalAnimatable.getCurrentKey()->transform.positionY;

            // Transform to part-space
            float pivotPSX = (partsTransformation.pivotX - keyPosX) / keyScaleX;
            float pivotPSY = (partsTransformation.pivotY - keyPosY) / keyScaleY;

            float posAddedX = part.transform.positionX + partsTransformation.translateX;
            float posAddedY = part.transform.positionY + partsTransformation.translateY;

            float offsetX = (posAddedX - pivotPSX) * (1.f - partsTransformation.scaleX);
            float offsetY = (posAddedY - pivotPSY) * (1.f - partsTransformation.scaleY);

            part.transform.positionX = posAddedX - offsetX;
            part.transform.positionY = posAddedY - offsetY;

            part.transform.scaleX *= partsTransformation.scaleX;
            part.transform.scaleY *= partsTransformation.scaleY;

            float pX = pivotPSX - (part.regionW * part.transform.scaleX) / 2;
            float pY = pivotPSY - (part.regionH * part.transform.scaleY) / 2;

            // Compensation on position for transformation rotation.

            float dx = part.transform.positionX - pX;
            float dy = part.transform.positionY - pY;

            float cosAngle = cosf(partsTransformation.rotation * (M_PI / 180.f)); 
            float sinAngle = sinf(partsTransformation.rotation * (M_PI / 180.f));

            float rotatedX = dx * cosAngle - dy * sinAngle;
            float rotatedY = dx * sinAngle + dy * cosAngle;

            part.transform.positionX = rotatedX + pX;
            part.transform.positionY = rotatedY + pY;

            part.transform.angle += partsTransformation.rotation;
        }

        partsBounding = calculatePartsBounding(arrangement, quadRotation);
        partsBoundingCenter = AVERAGE_IMVEC2(partsBounding[0], partsBounding[2]);
        partsAnmSpaceCenter = ImVec2(
            (partsBoundingCenter.x - origin.x) / this->canvasZoom,
            (partsBoundingCenter.y - origin.y) / this->canvasZoom
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
                newPivotX = (float)(int)(pivotPoint.x - partsAnmSpaceCenter.x);
                newPivotY = (float)(int)(pivotPoint.y - partsAnmSpaceCenter.y);
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
                sessionManager.getCurrentSession()->currentCellanim,
                appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                arrangementBeforeMutation // Used as the new arrangement
            ));
        }
        else
            appState.clearSelectedParts();
    }

    // All drawing operations
    {
        bool isBackgroundLight = false;
        switch (this->gridType) {
        case GridType_None:
            isBackgroundLight = !AppState::getInstance().getDarkThemeEnabled();
            break;
        case GridType_Dark:
            isBackgroundLight = false;
            break;
        case GridType_Light:
            isBackgroundLight = true;
            break;
        case GridType_Custom: {
            const float lumi =
                .2126f * customGridColor.x +
                .7152f * customGridColor.y +
                .0722f * customGridColor.z;
            if (lumi > .5f)
                isBackgroundLight = true;
            else
                isBackgroundLight = false;
        } break;

        default:
            break;
        }

        drawList->PushClipRect(this->canvasTopLeft, canvasBottomRight, true);
        {
            // Draw grid
            if (this->enableGridLines && this->gridType != GridType_None) {
                const float GRID_STEP = 64.f * this->canvasZoom;

                constexpr uint32_t normalColor = IM_COL32(200, 200, 200, 40);
                constexpr uint32_t centerColorX = IM_COL32(255, 0, 0, 70);
                constexpr uint32_t centerColorY = IM_COL32(0, 255, 0, 70);

                for (
                    float x = fmodf(this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2), GRID_STEP);
                    x < this->canvasSize.x;
                    x += GRID_STEP
                ) {
                    bool centered = fabs((this->canvasTopLeft.x + x) - origin.x) < .01f;
                    drawList->AddLine(
                        { this->canvasTopLeft.x + x, this->canvasTopLeft.y },
                        { this->canvasTopLeft.x + x, canvasBottomRight.y },
                        centered ? centerColorX : normalColor
                    );
                }

                for (
                    float y = fmodf(this->canvasOffset.y + static_cast<int>(this->canvasSize.y / 2), GRID_STEP);
                    y < this->canvasSize.y;
                    y += GRID_STEP
                ) {
                    bool centered = fabs((this->canvasTopLeft.y + y) - origin.y) < .01f;
                    drawList->AddLine(
                        { this->canvasTopLeft.x, this->canvasTopLeft.y + y },
                        { canvasBottomRight.x, this->canvasTopLeft.y + y },
                        centered ? centerColorY : normalColor
                    );
                }
            }

            // Draw animatable
            {
                bool drawOnionSkin = appState.onionSkinState.enabled;
                bool drawUnder = appState.onionSkinState.drawUnder;

                if (drawOnionSkin && drawUnder) {
                    globalAnimatable.DrawOnionSkin(
                        drawList,
                        appState.onionSkinState.backCount,
                        appState.onionSkinState.frontCount,
                        appState.onionSkinState.rollOver,
                        appState.onionSkinState.opacity
                    );
                }

                globalAnimatable.Draw(drawList, this->allowOpacity);

                if (drawOnionSkin && !drawUnder) {
                    globalAnimatable.DrawOnionSkin(
                        drawList,
                        appState.onionSkinState.backCount,
                        appState.onionSkinState.frontCount,
                        appState.onionSkinState.rollOver,
                        appState.onionSkinState.opacity
                    );
                }
            }

            // Draw safe area if enabled
            if (this->showSafeArea) {
                ImVec2 boxTopLeft (
                    origin.x - ((RVL_SAFE_X * this->canvasZoom) / 2.f),
                    origin.y - ((RVL_SAFE_Y * this->canvasZoom) / 2.f)
                );
                ImVec2 boxBottomRight (
                    boxTopLeft.x + (RVL_SAFE_X * this->canvasZoom),
                    boxTopLeft.y + (RVL_SAFE_Y * this->canvasZoom)
                );

                uint32_t color = IM_COL32(0, 0, 0, this->safeAreaAlpha);

                drawList->AddRectFilled( // Top Box
                    canvasTopLeft,
                    { canvasBottomRight.x, boxTopLeft.y },
                    color
                );
                drawList->AddRectFilled( // Left Box
                    { canvasTopLeft.x, boxTopLeft.y },
                    { boxTopLeft.x, boxBottomRight.y },
                    color
                );
                drawList->AddRectFilled( // Right Box
                    { boxBottomRight.x, boxTopLeft.y },
                    { canvasBottomRight.x, boxBottomRight.y },
                    color
                );
                drawList->AddRectFilled( // Bottom Box
                    { canvasTopLeft.x, boxBottomRight.y },
                    canvasBottomRight,
                    color
                );
            }
        }
        drawList->PopClipRect();

        // Draw selection box + handles & pivot point.
        if (!noParts /* && appState.focusOnSelectedPart */) {
            const uint32_t colorLine = isBackgroundLight ? 0xFF000000 : 0xFFFFFFFF;
            const uint32_t colorFill = isBackgroundLight ? 0xFFFFFFFF : 0xFF000000;

            constexpr float lineThickness = 1.5f;
            constexpr float handleLineThickness = 2.f;

            constexpr float handleRadius = 4.5f;

            drawList->AddQuad(partsBounding[0], partsBounding[1], partsBounding[2], partsBounding[3], colorLine, lineThickness);

            drawList->AddQuad(myPartsBounding[0], myPartsBounding[1], myPartsBounding[2], myPartsBounding[3], colorLine, lineThickness);

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
                (pivotPoint.x * this->canvasZoom) + origin.x,
                (pivotPoint.y * this->canvasZoom) + origin.y
            };

            // Draw the pivot
            const bool pivotDrawActive = hoveringOverPivot || movingPivot;

            const uint32_t pivotColor = IM_COL32(72, 82, 163, pivotDrawActive ? 255 : .8f * 255);
        
            float pivotDrawRadius = pivotDrawActive ? pivotRadius + 1.f : pivotRadius;
            constexpr float pivotLineThickness = 2.25f;

            constexpr float epsilon = 1e-2f;

            if ((fabs(partsTransformation.pivotX) > epsilon || partsTransformation.pivotY > epsilon) && !partsTransformation.pivotIsWorld) {
                constexpr uint32_t pivotColorOld = IM_COL32(57, 57, 57, .3f * 255);
                ImVec2 displayPivotPointOld = {
                    (partsAnmSpaceCenter.x * this->canvasZoom) + origin.x,
                    (partsAnmSpaceCenter.y * this->canvasZoom) + origin.y
                };

                constexpr float pivotRadiusOld = pivotRadius - 1.f;

                drawList->AddCircleFilled(displayPivotPointOld, pivotRadiusOld, pivotColorOld);
                drawList->AddCircle(displayPivotPointOld, pivotRadiusOld, pivotColorOld, 0, 3.f);
            }

            drawList->AddCircleFilled(displayPivotPoint, pivotDrawRadius, 0xFFFFFFFF);
            drawList->AddCircle(displayPivotPoint, pivotDrawRadius, 0xFF000000, 0, pivotLineThickness);
        }

        this->DrawCanvasText();
    }

    if (partsTransformation.active)
        arrangement = arrangementBeforeMutation;

    ImGui::End();
}

void WindowCanvas::DrawCanvasText() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    uint32_t textColor { 0xFF000000 }; // Black
    switch (this->gridType) {
    case GridType_None:
        // White or Black
        textColor = AppState::getInstance().getDarkThemeEnabled() ? 0xFFFFFFFF : 0xFF000000;
        break;

    case GridType_Dark:
        textColor = 0xFFFFFFFF; // White
        break;

    case GridType_Light:
        textColor = 0xFF000000; // Black
        break;

    case GridType_Custom: {
        const float lumi =
            .2126f * customGridColor.x +
            .7152f * customGridColor.y +
            .0722f * customGridColor.z;
        if (lumi > .5f)
            textColor = 0xFF000000; // Black
        else
            textColor = 0xFFFFFFFF; // White
    } break;

    default:
        break;
    }

    const float textDrawLeft = this->canvasTopLeft.x + 10.f;
    float textDrawTop = this->canvasTopLeft.y + 5.f;

    constexpr float textDrawGap = 3.f;

    if ((this->canvasOffset.x != 0.f) || (this->canvasOffset.y != 0.f)) {
        std::string str;
        {
            std::ostringstream fmtStream;
            fmtStream <<
                "Offset: " <<
                std::to_string(this->canvasOffset.x) << ", " <<
                std::to_string(this->canvasOffset.y);
            
            str = fmtStream.str();
        }

        drawList->AddText(
            { textDrawLeft, textDrawTop },
            textColor, str.c_str()
        );

        textDrawTop += textDrawGap + ImGui::CalcTextSize(str.c_str()).y;
    }

    if (this->canvasZoom != 1.f) {
        std::string str;
        {
            std::ostringstream fmtStream;
            fmtStream <<
                "Zoom: " <<
                std::to_string(static_cast<int>(this->canvasZoom * 100.f)) <<
                "% (hold [Shift] to zoom faster)";

            str = fmtStream.str();
        }

        drawList->AddText(
            { textDrawLeft, textDrawTop },
            textColor, str.c_str()
        );

        textDrawTop += textDrawGap + ImGui::CalcTextSize(str.c_str()).y;
    }

    if (!AppState::getInstance().globalAnimatable.getDoesDraw(this->allowOpacity)) {
        const char* text = "Nothing to draw on this frame";

        drawList->AddText(
            { textDrawLeft, textDrawTop },
            textColor, text
        );

        textDrawTop += textDrawGap + ImGui::CalcTextSize(text).y;
    }
}
