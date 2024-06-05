#include "WindowSpritesheet.hpp"

#include "imgui.h"

#include <iostream>
#include "../SessionManager.hpp"

#include <cstdlib> // for std::system

#include <sstream>
#include <future>

#include <filesystem>

#include "../ConfigManager.hpp"

#include <tinyfiledialogs.h>

#define SHEET_ZOOM_TIME .3f // seconds

void WindowSpritesheet::RunEditor() {
    GET_CONFIG_MANAGER;

    // Some apps like Photoshop complain about relative paths
    std::string imagePath = std::filesystem::absolute(configManager.config.textureEditPath.c_str()).string();

    std::stringstream commandStream;
    commandStream <<
        "cmd.exe /c \"\"" <<
        configManager.config.imageEditorPath <<
        "\" \"" << imagePath << "\"\"";

    std::string command = commandStream.str();
    std::cout << "[WindowSpritesheet::RunEditor] Running command: " << command << std::endl;

    std::thread t([command]() {
        std::system(command.c_str());
    }); t.detach();
}

void WindowSpritesheet::Update() {
    GET_SESSION_MANAGER;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Spritesheet", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

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

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Draw bounding boxes of active part(s)", nullptr, this->drawBounding))
                this->drawBounding = !this->drawBounding;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            bool imageEditorDefined = !ConfigManager::getInstance().config.imageEditorPath.empty();

            if (ImGui::MenuItem("Edit in editing tool...", nullptr, false, imageEditorDefined)) {
                GET_SESSION_MANAGER;
                GET_CONFIG_MANAGER;

                if (sessionManager.getCurrentSession()->getCellanimSheet()->ExportToFile(configManager.config.textureEditPath.c_str())) {
                    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                    ImGui::OpenPopup("###DialogWaitForModifiedPNG");
                    ImGui::PopID();

                    this->RunEditor();
                } else {
                    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                    ImGui::OpenPopup("###DialogPNGExportFailed");
                    ImGui::PopID();
                }
            }
            if (ImGui::MenuItem("Replace with new image...", nullptr, false)) {
                const char* filterPatterns[] = { "*.png", "*.tga", ".psd", "*.bmp", "*.gif" };
                char* openFileDialog = tinyfd_openFileDialog(
                    "Select a replacement image file",
                    nullptr,
                    ARRAY_LENGTH(filterPatterns), filterPatterns,
                    "Image file (.png, .tga, .psd, .bmp, .gif)",
                    false
                );

                if (openFileDialog) {
                    GET_SESSION_MANAGER;

                    Common::Image* newImage = new Common::Image();
                    newImage->LoadFromFile(openFileDialog);

                    if (newImage->texture) {
                        sessionManager.getCurrentSession()->getCellanimSheet()->FreeTexture();
                        delete sessionManager.getCurrentSession()->getCellanimSheet();
                        sessionManager.getCurrentSession()->getCellanimSheet() = newImage;
                        
                        sessionManager.SessionChanged();

                        sessionManager.getCurrentSessionModified() |= true;
                    }
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Format")) {
            ImGui::Text("Current format: %s", TPL::getImageFormatName(sessionManager.getCurrentSession()->getCellanimSheet()->tplOutFormat));

            ImGui::Separator();

            ImGui::MenuItem("Re-encode..");

            ImGui::EndMenu();
        }

        {
            char textBuffer[32];
            sprintf(textBuffer, "Double-click to %s", this->sheetZoomEnabled ? "un-zoom" : "zoom");

            ImGui::SetCursorPosX(
                ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(textBuffer).x -
                ImGui::GetStyle().FramePadding.x
            );
            ImGui::TextUnformatted(textBuffer);
        }

        ImGui::EndMenuBar();
    }

    ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasBottomRight = ImVec2(canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y);

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

    GET_WINDOW_DRAWLIST;

    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions
    ImGui::InvisibleButton("CanvasInteractions", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle
    );

    const bool interactionHovered = ImGui::IsItemHovered();
    const bool interactionActive = ImGui::IsItemActive(); // Held

    // Inital sheet zoom
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && interactionHovered) {
        this->sheetZoomTriggered = true;
        this->sheetZoomTimer = static_cast<float>(glfwGetTime());
    }

    if (
        this->sheetZoomTriggered &&
        (
            static_cast<float>(glfwGetTime()) -
            this->sheetZoomTimer
        ) > SHEET_ZOOM_TIME
    ) {
        this->sheetZoomEnabled = !this->sheetZoomEnabled;
        this->sheetZoomTriggered = false;
    }

    // Dragging
    const float mousePanThreshold = -1.0f;
    bool draggingCanvas = this->sheetZoomEnabled && interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold)
    );

    ImVec2 imageRect = ImVec2(
        static_cast<float>(sessionManager.getCurrentSession()->getCellanimSheet()->width),
        static_cast<float>(sessionManager.getCurrentSession()->getCellanimSheet()->height)
    );

    float scale;
    Common::fitRectangle(imageRect, canvasSize, scale);

    ImVec2 canvasOffset;

    if (draggingCanvas) {
        GET_IMGUI_IO;

        this->sheetZoomedOffset.x += io.MouseDelta.x;
        this->sheetZoomedOffset.y += io.MouseDelta.y;

        this->sheetZoomedOffset.x = std::clamp<float>(this->sheetZoomedOffset.x, -(imageRect.x / 2), imageRect.x / 2);
        this->sheetZoomedOffset.y = std::clamp<float>(this->sheetZoomedOffset.y, -(imageRect.y / 2), imageRect.y / 2);
    }

    if (this->sheetZoomTriggered) {
        float rel = (glfwGetTime() - this->sheetZoomTimer) / SHEET_ZOOM_TIME;
        float rScale = (this->sheetZoomEnabled ? Common::EaseIn : Common::EaseOut)(
            this->sheetZoomEnabled ?
                1.f - rel :
                rel
        ) + 1.f;

        scale *= rScale;
        imageRect.x *= rScale;
        imageRect.y *= rScale;

        canvasOffset.x = this->sheetZoomedOffset.x * (rScale - 1.f);
        canvasOffset.y = this->sheetZoomedOffset.y * (rScale - 1.f);
    } else {
        float rScale = this->sheetZoomEnabled ? 2.f : 1.f;

        scale *= rScale;
        imageRect.x *= rScale;
        imageRect.y *= rScale;

        canvasOffset.x = this->sheetZoomEnabled ? this->sheetZoomedOffset.x : 0.f;
        canvasOffset.y = this->sheetZoomEnabled ? this->sheetZoomedOffset.y : 0.f;
    }

    ImVec2 imagePosition = ImVec2(
        (canvasTopLeft.x + (canvasSize.x - imageRect.x) * 0.5f) + canvasOffset.x,
        (canvasTopLeft.y + (canvasSize.y - imageRect.y) * 0.5f) + canvasOffset.y
    );

    drawList->AddImage(
        (void*)(intptr_t)sessionManager.getCurrentSession()->getCellanimSheet()->texture,
        imagePosition,
        { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
    );

    GET_APP_STATE;

    if (this->drawBounding) {
        GET_ANIMATABLE;

        RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

        for (uint16_t i = 0; i < arrangementPtr->parts.size(); i++) {
            if (appState.focusOnSelectedPart && i != appState.selectedPart)
                continue;

            RvlCellAnim::ArrangementPart* partPtr = &arrangementPtr->parts.at(i);

            uint16_t sourceRect[4] = { partPtr->regionX, partPtr->regionY, partPtr->regionW, partPtr->regionH };

            float dataDiffScaleX = static_cast<float>(globalAnimatable->texture->width) / globalAnimatable->cellanim->textureW;
            float dataDiffScaleY = static_cast<float>(globalAnimatable->texture->height) / globalAnimatable->cellanim->textureH;

            ImVec2 topLeftOffset = {
                (sourceRect[0] * scale * dataDiffScaleX) + imagePosition.x,
                (sourceRect[1] * scale * dataDiffScaleY) + imagePosition.y,
            };

            ImVec2 bottomRightOffset = {
                topLeftOffset.x + (sourceRect[2] * scale * dataDiffScaleX),
                topLeftOffset.y + (sourceRect[3] * scale * dataDiffScaleY)
            };

            drawList->AddRect(topLeftOffset, bottomRightOffset, IM_COL32(255, 0, 0, 255));
        }
    }

    ImGui::End();
}