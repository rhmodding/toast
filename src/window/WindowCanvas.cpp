#include "WindowCanvas.hpp"

const ImVec2 feverSafeAreaSize(832, 456);
const ImVec2 megamixSafeAreaSize(440, 240);

const uint8_t uint8_one = 1;

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
                        gridType = AppState::getInstance().darkTheme ? GridType_Dark : GridType_Light;
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

                this->canvasZoom = scale - 1.f - 0.1f;
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

                bool pushDisable{ false };
                if (!this->visualizeSafeArea) {
                    pushDisable = true;
                    ImGui::BeginDisabled(true);
                }

                ImGui::DragScalar("Alpha", ImGuiDataType_U8, &this->safeAreaAlpha);

                if (pushDisable)
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    // Note: ImDrawList uses screen coordinates
    ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    this->canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasBottomRight = ImVec2(canvasTopLeft.x + this->canvasSize.x, canvasTopLeft.y + this->canvasSize.y);

    this->Menubar();

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
            break;
    }

    GET_IMGUI_IO;
    GET_WINDOW_DRAWLIST;

    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions
    ImGui::InvisibleButton("CanvasInteractions", this->canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle
    );

    const bool interactionHovered = ImGui::IsItemHovered();
    const bool interactionActive = ImGui::IsItemActive(); // Held

    const ImVec2 origin(
        canvasTopLeft.x + this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2),
        canvasTopLeft.y + this->canvasOffset.y + static_cast<int>(this->canvasSize.y / 2)
    );
    const ImVec2 relativeMousePos(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

    // Panning
    const float mousePanThreshold = -1.0f;
    bool panningCanvas = interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold)
    );

    if (panningCanvas) {
        this->canvasOffset.x += io.MouseDelta.x;
        this->canvasOffset.y += io.MouseDelta.y;
    }

    const float maxZoom = 9.f;
    const float minZoom = -0.90f;

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

    // Draw grid + all lines in the canvas
    drawList->PushClipRect(canvasTopLeft, canvasBottomRight, true);

    if ((this->gridType != GridType_None) && this->enableGridLines) {
        float GRID_STEP = 64.0f * (this->canvasZoom + 1);

        for (float x = fmodf(this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2), GRID_STEP); x < this->canvasSize.x; x += GRID_STEP)
            drawList->AddLine(ImVec2(canvasTopLeft.x + x, canvasTopLeft.y), ImVec2(canvasTopLeft.x + x, canvasBottomRight.y), IM_COL32(200, 200, 200, 40));
        
        for (float y = fmodf(this->canvasOffset.y + static_cast<int>(this->canvasSize.y / 2), GRID_STEP); y < this->canvasSize.y; y += GRID_STEP)
            drawList->AddLine(ImVec2(canvasTopLeft.x, canvasTopLeft.y + y), ImVec2(canvasBottomRight.x, canvasTopLeft.y + y), IM_COL32(200, 200, 200, 40));
    }

    GET_ANIMATABLE;

    bool animatableDoesDraw = globalAnimatable->getDoesDraw(allowOpacity);

    if (animatableDoesDraw) {
        globalAnimatable->scaleX = this->canvasZoom + 1;
        globalAnimatable->scaleY = this->canvasZoom + 1;

        globalAnimatable->offset = origin;
        globalAnimatable->Draw(drawList, allowOpacity);
    }

    GET_APP_STATE;

    if (appState.drawSelectedPartBounding && appState.selectedPart >= 0) {
        ImVec2* bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), appState.selectedPart);
        drawList->AddQuad(bounding[0], bounding[1], bounding[2], bounding[3], IM_COL32(
            partBoundingDrawColor.x * 255,
            partBoundingDrawColor.y * 255,
            partBoundingDrawColor.z * 255,
            partBoundingDrawColor.w * 255
        ));
        delete[] bounding;
    }

    if (this->drawAllBounding) {
        RvlCellAnim::Arrangement* arrangementPtr =
            &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

        for (uint16_t i = 0; i < arrangementPtr->parts.size(); i++) {
            uint32_t color = IM_COL32(
                partBoundingDrawColor.x * 255,
                partBoundingDrawColor.y * 255,
                partBoundingDrawColor.z * 255,
                partBoundingDrawColor.w * 255
            );

            if (appState.drawSelectedPartBounding && appState.selectedPart == i)
                color = IM_COL32(255, 255, 0, 255);

            ImVec2* bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), i);

            drawList->AddQuad(bounding[0], bounding[1], bounding[2], bounding[3], color);

            delete[] bounding;
        }
    }

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

    drawList->PopClipRect();

    uint32_t textColor{ 0 };
    switch (this->gridType) {
        case GridType_None:
            textColor = AppState::getInstance().darkTheme ? IM_COL32_WHITE : IM_COL32_BLACK;
            break;
        
        case GridType_Dark:
            textColor = IM_COL32_WHITE;
            break;

        case GridType_Light:
            textColor = IM_COL32_BLACK;
            break;

        case GridType_Custom: {
            float y = 0.2126f * customGridColor.x + 0.7152f * customGridColor.y + 0.0722f * customGridColor.z;

            if (y > 0.5f)
                textColor = IM_COL32_BLACK;
            else
                textColor = IM_COL32_WHITE;
        } break;
    
        default:
            break;
    }

    float textDrawHeight = canvasTopLeft.y + 5.f;

    if ((this->canvasOffset.x != 0.f) || (this->canvasOffset.y != 0.f)) {
        char buffer[64] = { '\0' };
        sprintf_s((char*)&buffer, 64, "Offset: %f, %f", this->canvasOffset.x, this->canvasOffset.y);

        drawList->AddText(ImVec2(canvasTopLeft.x + 10, textDrawHeight), textColor, buffer);

        textDrawHeight += 3.f + ImGui::CalcTextSize(buffer).y;
    }

    if (this->canvasZoom != 0.f) {
        char buffer[64] = { '\0' };
        sprintf_s((char*)&buffer, 64, "Zoom: %u%% (hold [Shift] to zoom faster)", static_cast<uint16_t>((this->canvasZoom + 1) * 100));

        drawList->AddText(
            ImVec2(
                canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, buffer
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(buffer).y;
    }

    if (!animatableDoesDraw) {
        const char* text = "Nothing to draw on this frame";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        drawList->AddText(
            ImVec2(
                canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, text
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(text).y;
    }

    ImGui::End();
}