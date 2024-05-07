#include "WindowSpritesheet.hpp"

#include "imgui.h"

#include <iostream>
#include "../SessionManager.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../stb/stb_image_write.h"

#include <cstdlib> // for std::system

#include <sstream>
#include <future>

#include "../ConfigManager.hpp"

#include <ImGuiFileDialog.h>

#include <filesystem>

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

                    std::string& imageEditorPath = ConfigManager::getInstance().config.imageEditorPath;
                    // Some apps like Photoshop complain about relative paths
                    std::string imagePath = std::filesystem::absolute(configManager.config.textureEditPath.c_str()).string();

                    std::stringstream commandStream;
                    commandStream << "cmd.exe /c \"\"" << imageEditorPath << "\" \"" << imagePath << "\"\"";

                    std::string command = commandStream.str();
                    std::cout << "" << command << std::endl;

                    std::thread t([command]() {
                        std::system(command.c_str());
                    });
                    t.detach();
                } else {
                    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                    ImGui::OpenPopup("###DialogPNGExportFailed");
                    ImGui::PopID();
                }
            }
            if (ImGui::MenuItem("Replace with PNG file...", nullptr, false)) {
                IGFD::FileDialogConfig dialogConfig;
                dialogConfig.path = ConfigManager::getInstance().config.lastFileDialogPath;
                dialogConfig.countSelectionMax = 1;
                dialogConfig.flags = ImGuiFileDialogFlags_Modal;

                ImGuiFileDialog::Instance()->OpenDialog("DialogReplaceCurrentTexturePNG", "Choose the replacement texture PNG", ".png", dialogConfig);
            }

            ImGui::EndMenu();
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

    ImVec2 imageRect = ImVec2(
        static_cast<float>(sessionManager.getCurrentSession()->getCellanimSheet()->width),
        static_cast<float>(sessionManager.getCurrentSession()->getCellanimSheet()->height)
    );

    float scale;
    Common::fitRectangle(imageRect, canvasSize, scale);

    ImVec2 imagePosition = ImVec2(
        ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - imageRect.x) * 0.5f,
        ImGui::GetCursorScreenPos().y + (ImGui::GetContentRegionAvail().y - imageRect.y) * 0.5f
    );

    drawList->AddImage(
        (void*)(intptr_t)sessionManager.getCurrentSession()->getCellanimSheet()->texture,
        imagePosition,
        { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
    );

    GET_APP_STATE;

    if (this->drawBounding) {
        GET_ANIMATABLE;

        RvlCellAnim::Arrangement* arrangementPtr =
            &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

        for (uint16_t i = 0; i < arrangementPtr->parts.size(); i++) {
            if (appState.focusOnSelectedPart && i != appState.selectedPart)
                continue;

            RvlCellAnim::ArrangementPart* partPtr = &arrangementPtr->parts.at(i);

            uint16_t sourceRect[4] = { partPtr->regionX, partPtr->regionY, partPtr->regionW, partPtr->regionH };

            ImVec2 topLeftOffset = {
                (sourceRect[0] * scale) + imagePosition.x,
                (sourceRect[1] * scale) + imagePosition.y,
            };

            ImVec2 bottomRightOffset = {
                topLeftOffset.x + (sourceRect[2] * scale),
                topLeftOffset.y + (sourceRect[3] * scale)
            };

            drawList->AddRect(topLeftOffset, bottomRightOffset, IM_COL32(255, 0, 0, 255));
        }
    }

    ImGui::End();
}