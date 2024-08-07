#include "WindowCanvas.hpp"

#include <sstream>
#include <string>

#include <cmath>

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"
#include "../PlayerManager.hpp"

#include "../command/CommandModifyArrangementPart.hpp"

#include "../common.hpp"

bool isPointInPolygon(const ImVec2& point, const ImVec2* polygon, uint32_t numVertices) {
    float x = point.x, y = point.y;
    bool inside{ false };

    ImVec2 p1 = polygon[0];
    ImVec2 p2;

    for (uint32_t i = 1; i <= numVertices; i++) {
        p2 = polygon[i % numVertices];

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

void DrawRotatedBox(ImDrawList* draw_list, ImVec2 center, float radius, float rotation, ImU32 color) {
    float radiusInv = -radius;

    auto rotatePoint = [](ImVec2 point, ImVec2 center, float rotation) -> ImVec2 {
        if (fmod(rotation, 360.f) == 0.f)
            return point;

        float rad = rotation * IM_PI / 180.f;

        float s = sinf(rad);
        float c = cosf(rad);

        point.x -= center.x;
        point.y -= center.y;

        float xnew = point.x * c - point.y * s;
        float ynew = point.x * s + point.y * c;

        point.x = xnew + center.x;
        point.y = ynew + center.y;

        return point;
    };

    draw_list->AddQuad(
        rotatePoint({
            radiusInv + center.x,
            radiusInv + center.y
        }, center, rotation),
        rotatePoint({
            radius    + center.x,
            radiusInv + center.y
        }, center, rotation),
        rotatePoint({
            radius + center.x,
            radius + center.y
        }, center, rotation),
        rotatePoint({
            radiusInv + center.x,
            radius    + center.y
        }, center, rotation),

        color
    );
}

const ImVec2 feverSafeAreaSize(832, 456);
const ImVec2 megamixSafeAreaSize(440, 240);

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
                this->canvasZoom = 0.f;
            }

            if (ImGui::MenuItem("Reset to Safe Area", nullptr, false)) {
                this->canvasOffset = { 0.f, 0.f };

                ImVec2 rect = feverSafeAreaSize;

                float scale;
                Common::fitRectangle(rect, this->canvasSize, scale);

                this->canvasZoom = scale - 1.f - .1f;
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
                ImGui::Checkbox("Enabled", &this->visualizeSafeArea);

                ImGui::SeparatorText("Options");

                ImGui::BeginDisabled(!this->visualizeSafeArea);
                ImGui::DragScalar("Alpha", ImGuiDataType_U8, &this->safeAreaAlpha);
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Enable part transparency", nullptr, allowOpacity))
                allowOpacity = !allowOpacity;

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void WindowCanvas::Update() {
    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    // Note: ImDrawList uses screen coordinates
    this->canvasTopLeft = ImGui::GetCursorScreenPos();
    this->canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasBottomRight = ImVec2(
        this->canvasTopLeft.x + this->canvasSize.x,
        this->canvasTopLeft.y + this->canvasSize.y
    );

    this->Menubar();

    // Determine background color
    uint32_t backgroundColor{ 0 };
    switch (this->gridType) {
        case GridType_None:
            backgroundColor = IM_COL32_BLACK_TRANS;
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
            // The default value of backgroundColor is already 0.
            break;
    }

    GET_IMGUI_IO;
    GET_WINDOW_DRAWLIST;

    drawList->AddRectFilled(this->canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions
    ImGui::InvisibleButton("CanvasInteractions", this->canvasSize,
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
    const float mousePanThreshold = -1.f;
    bool draggingLeft = interactionActive &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold);

    bool draggingCanvas = interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle, mousePanThreshold) ||
        (
            ConfigManager::getInstance().getConfig().canvasLMBPanEnabled &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold)
        )
    );

    hoveredPartHandle = PartHandle_None;

    static PartHandle activePartHandle{ PartHandle_None };

    static RvlCellAnim::ArrangementPart partBeforeInteraction;

    static bool panningCanvas{ false };

    static ImVec2  dragPartOffset{ 0, 0 };
    static ImVec2 newPartSize{ 0, 0 };

    static bool hoveringOverSelectedPart{ false };

    GET_ANIMATABLE;
    GET_APP_STATE;

    RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

    const bool partLocked = appState.selectedPart >= 0 ?
        arrangementPtr->parts.at(appState.selectedPart).editorLocked :
        false;
    const bool partVisible = appState.selectedPart >= 0 ?
        arrangementPtr->parts.at(appState.selectedPart).editorVisible :
        true;

    {
        globalAnimatable->offset = origin;

        globalAnimatable->scaleX = this->canvasZoom + 1;
        globalAnimatable->scaleY = this->canvasZoom + 1;
    }

    // All drawing operations (and handle calculation)
    {
        drawList->PushClipRect(this->canvasTopLeft, canvasBottomRight, true);
        {
            // Draw grid
            if ((this->gridType != GridType_None) && this->enableGridLines) {
                const float GRID_STEP = 64.f * (this->canvasZoom + 1.f);

                static const ImU32 normalColor = IM_COL32(200, 200, 200, 40);
                static const ImU32 centerColorX = IM_COL32(255, 0, 0, 70);
                static const ImU32 centerColorY = IM_COL32(0, 255, 0, 70);

                for (
                    float x = fmodf(this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2), GRID_STEP);
                    x < this->canvasSize.x;
                    x += GRID_STEP
                ) {
                    bool centered = fabs((this->canvasTopLeft.x + x) - origin.x) < .01f;
                    drawList->AddLine(
                        ImVec2(this->canvasTopLeft.x + x, this->canvasTopLeft.y),
                        ImVec2(this->canvasTopLeft.x + x, canvasBottomRight.y),
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
                        ImVec2(this->canvasTopLeft.x, this->canvasTopLeft.y + y),
                        ImVec2(canvasBottomRight.x, this->canvasTopLeft.y + y),
                        centered ? centerColorY : normalColor
                    );
                }
            }

            // Draw animatable
            {
                bool drawOnionSkin = appState.onionSkinState.enabled;
                bool drawUnder = appState.onionSkinState.drawUnder;

                if (drawOnionSkin && drawUnder) {
                    globalAnimatable->DrawOnionSkin(
                        drawList,
                        appState.onionSkinState.backCount,
                        appState.onionSkinState.frontCount,
                        appState.onionSkinState.opacity
                    );
                }

                globalAnimatable->Draw(drawList, allowOpacity);

                if (drawOnionSkin && !drawUnder) {
                    globalAnimatable->DrawOnionSkin(
                        drawList,
                        appState.onionSkinState.backCount,
                        appState.onionSkinState.frontCount,
                        appState.onionSkinState.opacity
                    );
                }
            }

            // Draw safe area if enabled
            if (this->visualizeSafeArea) {
                ImVec2 boxTopLeft = {
                    origin.x - ((feverSafeAreaSize.x * (this->canvasZoom + 1)) / 2),
                    origin.y - ((feverSafeAreaSize.y * (this->canvasZoom + 1)) / 2)
                };

                ImVec2 boxBottomRight = {
                    boxTopLeft.x + (feverSafeAreaSize.x * (this->canvasZoom + 1)),
                    boxTopLeft.y + (feverSafeAreaSize.y * (this->canvasZoom + 1))
                };

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

            // Draw selected part bounds
            if (appState.focusOnSelectedPart && appState.selectedPart >= 0) {
                uint32_t color = hoveringOverSelectedPart && ImGui::IsWindowHovered() ?
                    IM_COL32(255, 255, 0, 255) :
                    IM_COL32(
                        partBoundingDrawColor.x * 255,
                        partBoundingDrawColor.y * 255,
                        partBoundingDrawColor.z * 255,
                        partBoundingDrawColor.w * 255
                    );

                auto bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), appState.selectedPart);
                drawList->AddQuad(
                    ROUND_IMVEC2(bounding[0]), ROUND_IMVEC2(bounding[1]),
                    ROUND_IMVEC2(bounding[2]), ROUND_IMVEC2(bounding[3]),
                    color
                );

                float thrLength = sqrtf(
                    ((bounding[2].x - bounding[0].x)*(bounding[2].x - bounding[0].x)) +
                    ((bounding[2].y - bounding[0].y)*(bounding[2].y - bounding[0].y))
                );

                // If quad is too small or the part is locked, only draw the bounding.
                if (thrLength >= 35.f && !partLocked) {
                    float angle =
                        globalAnimatable->getCurrentArrangement()->parts.at(appState.selectedPart).angle +
                        globalAnimatable->getCurrentKey()->angle;

                    ImVec2 point;

                    // Side boxes
                    for (uint32_t i = 0; i < 4; i++) {
                        point = AVERAGE_IMVEC2_ROUND(bounding[i], bounding[(i+1) % 4]);

                        if (Common::IsMouseInRegion(point))
                            *(short*)&hoveredPartHandle = PartHandle_Top + i;

                        DrawRotatedBox(drawList, point, 4.f, angle, color);
                    }

                    for (uint32_t i = 0; i < 4; i++) {
                        point = ROUND_IMVEC2(bounding[i]);

                        if (Common::IsMouseInRegion(point))
                            *(short*)&hoveredPartHandle = PartHandle_TopLeft + i;

                        DrawRotatedBox(drawList, point, 4.f, angle, color);
                    }

                    point = AVERAGE_IMVEC2_ROUND(bounding[0], bounding[2]);

                    // Center box
                    DrawRotatedBox(drawList, point, 3.f, angle, color);
                }
            }

            // Draw all part bounds if enabled
            if (this->drawAllBounding)
                for (uint16_t i = 0; i < arrangementPtr->parts.size(); i++) {
                    if (appState.focusOnSelectedPart && appState.selectedPart == i)
                        continue; // Skip over part if bounds are already drawn

                    const uint32_t color = IM_COL32(
                        partBoundingDrawColor.x * 255,
                        partBoundingDrawColor.y * 255,
                        partBoundingDrawColor.z * 255,
                        partBoundingDrawColor.w * 255
                    );

                    auto bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), i);
                    drawList->AddQuad(
                        ROUND_IMVEC2(bounding[0]), ROUND_IMVEC2(bounding[1]),
                        ROUND_IMVEC2(bounding[2]), ROUND_IMVEC2(bounding[3]),
                        color
                    );
                }
        }
        drawList->PopClipRect();

        if (hoveredPartHandle == PartHandle_None && hoveringOverSelectedPart)
            hoveredPartHandle = PartHandle_Whole;

        if (partLocked || panningCanvas)
            hoveredPartHandle = PartHandle_None;

        this->DrawCanvasText();

        // Set tooltip
        if (hoveringOverSelectedPart && ImGui::IsWindowHovered() && !draggingCanvas) {
            std::ostringstream fmtStream;

            fmtStream << "Part no. %u\n";

            if (!partLocked)
                fmtStream << "You can drag this part to change it's position.";
            else
                fmtStream << "This part is locked.";

            if (!partVisible)
                fmtStream << "\nThis part is invisible.";

            ImGui::SetTooltip(
                fmtStream.str().c_str(),
                appState.selectedPart + 1
            );
        }
    }

    // set hoveringOverSelectedPart
    if (
        appState.focusOnSelectedPart &&
        appState.selectedPart >= 0 &&
        !PlayerManager::getInstance().playing
    ) {
        auto bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), appState.selectedPart);
        ImVec2 polygon[5] = {
            bounding[0],
            bounding[1],
            bounding[2],
            bounding[3],
            bounding[0]
        };

        hoveringOverSelectedPart = isPointInPolygon(io.MousePos, polygon, 5);
    }

    if (
        hoveredPartHandle != PartHandle_None &&
        activePartHandle == PartHandle_None &&
        draggingLeft
    ) {
        activePartHandle = hoveredPartHandle;

        partBeforeInteraction = arrangementPtr->parts.at(appState.selectedPart);

        newPartSize = ImVec2(
            arrangementPtr->parts.at(appState.selectedPart).regionW,
            arrangementPtr->parts.at(appState.selectedPart).regionH
        );

        if (hoveredPartHandle == PartHandle_Whole) {
            dragPartOffset = ImVec2(
                arrangementPtr->parts.at(appState.selectedPart).positionX,
                arrangementPtr->parts.at(appState.selectedPart).positionY
            );
        }
    }

    if (draggingCanvas && activePartHandle == PartHandle_None)
        panningCanvas = true;

    if (activePartHandle == PartHandle_Whole) {
        dragPartOffset.x +=
            io.MouseDelta.x / ((canvasZoom) + 1.f) /
            globalAnimatable->getCurrentKey()->scaleX;
        dragPartOffset.y +=
            io.MouseDelta.y / ((canvasZoom) + 1.f) /
            globalAnimatable->getCurrentKey()->scaleY;

        arrangementPtr->parts.at(appState.selectedPart).positionX =
            static_cast<int16_t>(dragPartOffset.x);
        arrangementPtr->parts.at(appState.selectedPart).positionY =
            static_cast<int16_t>(dragPartOffset.y);
    }

    bool caseBottom =
        activePartHandle == PartHandle_Bottom ||
        activePartHandle == PartHandle_BottomLeft ||
        activePartHandle == PartHandle_BottomRight;
    bool caseRight =
        activePartHandle == PartHandle_Right ||
        activePartHandle == PartHandle_BottomRight ||
        activePartHandle == PartHandle_TopRight;
    bool caseTop =
        activePartHandle == PartHandle_Top ||
        activePartHandle == PartHandle_TopLeft ||
        activePartHandle == PartHandle_TopRight;
    bool caseLeft =
        activePartHandle == PartHandle_Left ||
        activePartHandle == PartHandle_BottomLeft ||
        activePartHandle == PartHandle_TopLeft;

    bool flippedX{ false }, flippedY{ false };
    if (appState.selectedPart >= 0) {
        flippedX = arrangementPtr->parts.at(appState.selectedPart).flipX;
        flippedY = arrangementPtr->parts.at(appState.selectedPart).flipY;
    }

    if (flippedY ? caseTop : caseBottom) {
        newPartSize.y +=
            io.MouseDelta.y / (canvasZoom + 1.f) /
            globalAnimatable->getCurrentKey()->scaleY;

        arrangementPtr->parts.at(appState.selectedPart).scaleY = (
            newPartSize.y /
            arrangementPtr->parts.at(appState.selectedPart).regionH
        ) + partBeforeInteraction.scaleY - 1.f;
    }

    if (flippedX ? caseLeft : caseRight) {
        newPartSize.x +=
            io.MouseDelta.x / (canvasZoom + 1.f) /
            globalAnimatable->getCurrentKey()->scaleX;

        arrangementPtr->parts.at(appState.selectedPart).scaleX = (
            newPartSize.x /
            arrangementPtr->parts.at(appState.selectedPart).regionW
        ) + partBeforeInteraction.scaleX - 1.f;
    }

    if (flippedY ? caseBottom : caseTop) {
        newPartSize.y -=
            io.MouseDelta.y / (canvasZoom + 1.f) /
            globalAnimatable->getCurrentKey()->scaleY;

        arrangementPtr->parts.at(appState.selectedPart).scaleY = (
            newPartSize.y /
            arrangementPtr->parts.at(appState.selectedPart).regionH
        ) + partBeforeInteraction.scaleY - 1.f;

        arrangementPtr->parts.at(appState.selectedPart).positionY = (
            partBeforeInteraction.positionY - (
                newPartSize.y -
                arrangementPtr->parts.at(appState.selectedPart).regionH
            )
        );
    }

    if (flippedX ? caseRight : caseLeft) {
        newPartSize.x -=
            io.MouseDelta.x / (canvasZoom + 1.f) /
            globalAnimatable->getCurrentKey()->scaleX;

        arrangementPtr->parts.at(appState.selectedPart).scaleX = (
            newPartSize.x /
            arrangementPtr->parts.at(appState.selectedPart).regionW
        ) + partBeforeInteraction.scaleX - 1.f;

        arrangementPtr->parts.at(appState.selectedPart).positionX = (
            partBeforeInteraction.positionX - (
                newPartSize.x -
                arrangementPtr->parts.at(appState.selectedPart).regionW
            )
        );
    }

    if (panningCanvas) {
        this->canvasOffset.x += io.MouseDelta.x;
        this->canvasOffset.y += io.MouseDelta.y;
    }

    if (interactionDeactivated && panningCanvas)
        panningCanvas = false;

    if (interactionDeactivated && activePartHandle != PartHandle_None) { // Submit command
        changed = true;

        GET_SESSION_MANAGER;
        GET_ANIMATABLE;

        RvlCellAnim::ArrangementPart newPart = arrangementPtr->parts.at(appState.selectedPart);
        arrangementPtr->parts.at(appState.selectedPart) = partBeforeInteraction;

        sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandModifyArrangementPart>(newPart)
        );

        activePartHandle = PartHandle_None;
    }

    // Canvas zooming
    {
        const float maxZoom = 9.f;
        const float minZoom = -.90f;

        if (interactionHovered) {
            GET_IMGUI_IO;

            if (io.KeyShift)
                this->canvasZoom += io.MouseWheel * CANVAS_ZOOM_SPEED_FAST;
            else
                this->canvasZoom += io.MouseWheel * CANVAS_ZOOM_SPEED;

            if (this->canvasZoom < minZoom)
                this->canvasZoom = minZoom;

            if (this->canvasZoom > maxZoom)
                this->canvasZoom = maxZoom;
        }
    }

    ImGui::End();
}

void WindowCanvas::DrawCanvasText() {
    uint32_t textColor{ 0 };
    switch (this->gridType) {
        case GridType_None:
            textColor = AppState::getInstance().getDarkThemeEnabled() ? IM_COL32_WHITE : IM_COL32_BLACK;
            break;

        case GridType_Dark:
            textColor = IM_COL32_WHITE;
            break;

        case GridType_Light:
            textColor = IM_COL32_BLACK;
            break;

        case GridType_Custom: {
            float lumi = .2126f * customGridColor.x + .7152f * customGridColor.y + .0722f * customGridColor.z;
            if (lumi > .5f)
                textColor = IM_COL32_BLACK;
            else
                textColor = IM_COL32_WHITE;
        } break;

        default:
            break;
    }

    float textDrawHeight = this->canvasTopLeft.y + 5.f;

    GET_WINDOW_DRAWLIST;

    if ((this->canvasOffset.x != 0.f) || (this->canvasOffset.y != 0.f)) {
        std::ostringstream fmtStream;
        fmtStream <<
            "Offset: " <<
            std::to_string(this->canvasOffset.x) << ", " <<
            std::to_string(this->canvasOffset.y);

        drawList->AddText(
            ImVec2(
                this->canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, fmtStream.str().c_str()
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(fmtStream.str().c_str()).y;
    }

    if (this->canvasZoom != 0.f) {
        std::ostringstream fmtStream;
        fmtStream <<
            "Zoom: " <<
            std::to_string(static_cast<uint16_t>((this->canvasZoom + 1) * 100)) <<
            "% (hold [Shift] to zoom faster)";

        drawList->AddText(
            ImVec2(
                this->canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, fmtStream.str().c_str()
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(fmtStream.str().c_str()).y;
    }

    if (!AppState::getInstance().globalAnimatable->getDoesDraw(this->allowOpacity)) {
        const char* text = "Nothing to draw on this frame";

        drawList->AddText(
            ImVec2(
                this->canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, text
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(text).y;
    }

    /*

    char text[64];

    sprintf(text, "Current handle: %i", hoveredPartHandle);

    drawList->AddText(
        ImVec2(
            this->canvasTopLeft.x + 10,
            textDrawHeight
        ),
        textColor, text
    );

    */
}
