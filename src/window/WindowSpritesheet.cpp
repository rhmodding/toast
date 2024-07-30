#include "WindowSpritesheet.hpp"

#include <imgui.h>

#include <cstdlib>

#include <iostream>
#include <sstream>

#include <string>

#include <tinyfiledialogs.h>

#include <filesystem>

#include <thread>

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"

#include "../texture/ImageConvert.hpp"

#define SHEET_ZOOM_TIME .3f // seconds

void WindowSpritesheet::RunEditor() {
    GET_CONFIG_MANAGER;

    // Some apps like Photoshop complain about relative paths
    std::string imagePath = std::filesystem::absolute(configManager.getConfig().textureEditPath.c_str()).string();

    std::ostringstream commandStream;
    commandStream <<
        "cmd.exe /c \"\"" <<
        configManager.getConfig().imageEditorPath <<
        "\" \"" << imagePath << "\"\"";

    std::string command = commandStream.str();
    std::cout << "[WindowSpritesheet::RunEditor] Running command: " << command << std::endl;

    std::thread t([command]() {
        std::system(command.c_str());
    }); t.detach();
}

void WindowSpritesheet::PaletteWindow() {
    GET_SESSION_MANAGER;
    auto& colors = sessionManager.getCurrentSession()->getCellanimSheet()->tplColorPalette;

    if (colors.empty())
        this->showPaletteWindow = false;

    if (!this->showPaletteWindow)
        return;

    if (ImGui::Begin("Color Palette", &this->showPaletteWindow)) {
        GET_WINDOW_DRAWLIST;

        static uint16_t selected{ 0 };
        selected = std::clamp<uint16_t>(selected, 0, colors.size() - 1);

        // Left
        {
            ImGui::BeginChild("ColorList", ImVec2(250, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
            for (uint16_t i = 0; i < colors.size(); i++) {
                const uint32_t& color = colors.at(i);

                const uint8_t r = (color >> 24) & 0xFF;
                const uint8_t g = (color >> 16) & 0xFF;
                const uint8_t b = (color >>  8) & 0xFF;
                const uint8_t a = (color >>  0) & 0xFF;

                char buffer[48];
                sprintf(buffer, "[%03u] - R: %03u G: %03u B: %03u A: %03u", i, r, g, b, a);

                ImGui::PushID(i);

                if (ImGui::Selectable(buffer, selected == i))
                    selected = i;

                ImGui::SameLine();

                ImVec4 vectorColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
                ImGui::ColorButton("Palette Color", vectorColor, ImGuiColorEditFlags_Uint8, ImVec2(18, 18));

                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        // Right
        {
            ImGui::BeginChild("Properties");

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, -2 });

            ImGui::PushFont(AppState::getInstance().fonts.large);
            ImGui::TextWrapped("Color %u", selected);
            ImGui::PopFont();

            ImGui::PopStyleVar();

            ImGui::Dummy({ 0, 5 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            uint32_t& selectedColor = colors.at(selected);

            const uint8_t r = (selectedColor >> 24) & 0xFF;
            const uint8_t g = (selectedColor >> 16) & 0xFF;
            const uint8_t b = (selectedColor >>  8) & 0xFF;
            const uint8_t a = (selectedColor >>  0) & 0xFF;

            float values[4] = { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
            if (ImGui::ColorPicker4(
                "##ColorPicker",
                values,
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                nullptr
            ))
                selectedColor = (
                    (static_cast<uint8_t>(values[0] * 255.f) << 24) |
                    (static_cast<uint8_t>(values[1] * 255.f) << 16) |
                    (static_cast<uint8_t>(values[2] * 255.f) <<  8) |
                    (static_cast<uint8_t>(values[3] * 255.f) <<  0)
                );


            ImGui::EndChild();
        }
    }

    ImGui::End();
}

void WindowSpritesheet::FormatPopup() {
    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Re-encode image..###ReencodePopup", nullptr)) {
        GET_SESSION_MANAGER;

        const char* formatList[] = {
            "RGBA32",
            "RGB5A3",
        };
        static int selectedFormatIndex{ 0 };

        TPL::TPLImageFormat tplFormat;
        switch (selectedFormatIndex) {
            case 0:
                tplFormat = TPL::TPL_IMAGE_FORMAT_RGBA32;
                break;
            case 1:
                tplFormat = TPL::TPL_IMAGE_FORMAT_RGB5A3;
                break;

            default:
                tplFormat = TPL::TPL_IMAGE_FORMAT_RGBA32;
                break;
        }

        // Left
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("Properties", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);

            {
                ImGui::PushFont(AppState::getInstance().fonts.large);
                ImGui::SeparatorText("Image Info");
                ImGui::PopFont();

                if (ImGui::BeginChild("ImageInfoChild", ImVec2(-FLT_MIN, 0.f), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY)) {
                    ImGui::BulletText(
                        "Size: %ux%u",
                        sessionManager.getCurrentSession()->getCellanimSheet()->width,
                        sessionManager.getCurrentSession()->getCellanimSheet()->height
                    );

                    uint32_t dataSize = ImageConvert::getImageByteSize(
                        tplFormat,
                        sessionManager.getCurrentSession()->getCellanimSheet()->width,
                        sessionManager.getCurrentSession()->getCellanimSheet()->height
                    );
                    char formattedStr[32];
                    {
                        char numberStr[32];
                        sprintf(numberStr, "%u", dataSize);

                        uint32_t srcLen = strlen(numberStr);
                        uint32_t destLen = srcLen + (srcLen - 1) / 3;

                        int srcIndex = srcLen - 1;
                        int destIndex = destLen - 1;

                        uint16_t digitCount = 0;

                        while (srcIndex >= 0) {
                                if (digitCount == 3) {
                                formattedStr[destIndex--] = '.';
                                digitCount = 0;
                                }
                                else {
                                formattedStr[destIndex--] = numberStr[srcIndex--]; // Fix here
                                digitCount++;
                                }
                        }
                        formattedStr[destLen] = '\0'; // Null-terminate the new string
                    }

                    ImGui::BulletText("Data Size: %sB", formattedStr);
                }
                ImGui::EndChild();
            }
            {
                ImGui::PushFont(AppState::getInstance().fonts.large);
                ImGui::SeparatorText("Format Info");
                ImGui::PopFont();

                if (ImGui::BeginChild("FormatInfoChild", ImVec2(-FLT_MIN, 0.f), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY)) {
                    const char* colorDesc;
                    const char* alphaDesc;

                    switch (tplFormat) {
                        case TPL::TPL_IMAGE_FORMAT_RGBA32:
                            colorDesc = "Colors: 24-bit, millions of colors";
                            alphaDesc = "Alpha: 8-bit, ranged 0 to 255";
                            break;
                        case TPL::TPL_IMAGE_FORMAT_RGB5A3:
                            colorDesc = "Color:\n   Opaque pixels:\n      15-bit, 32768 colors\n   Transparent pixels:\n      12-bit, 4096 colors";
                            alphaDesc = "Alpha: 3-bit, ranged 0 to 7";
                            break;
                        default:
                            colorDesc = "";
                            alphaDesc = "";
                            break;
                    }

                    ImGui::BulletText("%s", colorDesc);
                    ImGui::BulletText("%s", alphaDesc);
                }
                ImGui::EndChild();
            }

            ImGui::Dummy({ 0, 5 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            ImGui::Combo("Format", &selectedFormatIndex, formatList, ARRAY_LENGTH(formatList));

            ImGui::EndChild();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();

            if (ImGui::Button("Apply")) {
                sessionManager.getCurrentSession()->getCellanimSheet()->tplOutFormat = tplFormat;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndGroup();
        }

        ImGui::SameLine();

        // Right
        {
            ImGui::SetNextWindowSizeConstraints({ 256.f, 0.f }, { FLT_MAX, FLT_MAX });
            ImGui::BeginChild("ImageView");

            static float bgScale{ .215f };

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##BGSlider", &bgScale, 0.f, 1.f, "BG: %.3f");

            ImVec2 imageRect = ImVec2(
                static_cast<float>(sessionManager.getCurrentSession()->getCellanimSheet()->width),
                static_cast<float>(sessionManager.getCurrentSession()->getCellanimSheet()->height)
            );

            float scale;
            Common::fitRectangle(imageRect, ImGui::GetContentRegionAvail(), scale);

            ImVec2 imagePosition = ImVec2(
                ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - imageRect.x) * 0.5f,
                ImGui::GetCursorScreenPos().y + (ImGui::GetContentRegionAvail().y - imageRect.y) * 0.5f
            );

            ImGui::GetWindowDrawList()->AddRectFilled(
                imagePosition,
                { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, },
                ImGui::ColorConvertFloat4ToU32({ bgScale, bgScale, bgScale, 1.f })
            );

            ImGui::GetWindowDrawList()->AddImage(
                (void*)(intptr_t)sessionManager.getCurrentSession()->getCellanimSheet()->texture,
                imagePosition,
                { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
            );

            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

void WindowSpritesheet::Update() {
    GET_SESSION_MANAGER;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Spritesheet", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImVec2 windowSize = ImGui::GetContentRegionAvail();

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

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Draw bounding boxes of active part(s)", nullptr, this->drawBounding))
                this->drawBounding = !this->drawBounding;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            bool imageEditorDefined = !ConfigManager::getInstance().getConfig().imageEditorPath.empty();

            if (ImGui::MenuItem("Edit in editing tool...", nullptr, false, imageEditorDefined)) {
                GET_SESSION_MANAGER;
                GET_CONFIG_MANAGER;

                if (sessionManager.getCurrentSession()->getCellanimSheet()->ExportToFile(configManager.getConfig().textureEditPath.c_str())) {
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

                    std::shared_ptr<Common::Image> newImage =
                        std::make_shared<Common::Image>(Common::Image());
                    newImage->LoadFromFile(openFileDialog);

                    if (newImage->texture) {
                        auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

                        bool diffSize =
                            newImage->width  != cellanimSheet->width ||
                            newImage->height != cellanimSheet->height;

                        cellanimSheet = newImage;

                        if (diffSize) {
                            ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                            ImGui::OpenPopup("###DialogModifiedPNGSizeDiff");
                            ImGui::PopID();
                        }

                        sessionManager.SessionChanged();
                        sessionManager.getCurrentSessionModified() = true;
                    }
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Data")) {
            auto imageFormat = sessionManager.getCurrentSession()->getCellanimSheet()->tplOutFormat;

            ImGui::Text("Image Format: %s", TPL::getImageFormatName(imageFormat));

            ImGui::Separator();

            if (ImGui::MenuItem("Re-encode (change format)..")) {
                ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                ImGui::OpenPopup("###ReencodePopup");
                ImGui::PopID();
            }

            if (ImGui::MenuItem(
                "Modify color palette..",
                nullptr, nullptr,
                imageFormat == TPL::TPL_IMAGE_FORMAT_C4 ||
                imageFormat == TPL::TPL_IMAGE_FORMAT_C8 ||
                imageFormat == TPL::TPL_IMAGE_FORMAT_C14X2
            ))
                this->showPaletteWindow = true;

            ImGui::EndMenu();
        }

        {
            char textBuffer[32];
            sprintf(textBuffer, "Double-click to %s", this->sheetZoomEnabled ? "un-zoom" : "zoom");

            ImGui::SetCursorPosX(
                windowSize.x - ImGui::CalcTextSize(textBuffer).x -
                ImGui::GetStyle().FramePadding.x
            );
            ImGui::TextUnformatted(textBuffer);
        }

        ImGui::EndMenuBar();
    }

    this->FormatPopup();

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
        this->sheetZoomTimer = static_cast<float>(ImGui::GetTime());
    }

    if (
        this->sheetZoomTriggered &&
        (
            static_cast<float>(ImGui::GetTime()) -
            this->sheetZoomTimer
        ) > SHEET_ZOOM_TIME
    ) {
        this->sheetZoomEnabled = !this->sheetZoomEnabled;
        this->sheetZoomTriggered = false;
    }

    // Dragging
    const float mousePanThreshold = -1.f;
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
        float rel = (ImGui::GetTime() - this->sheetZoomTimer) / SHEET_ZOOM_TIME;
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

            float mismatchScaleX = static_cast<float>(globalAnimatable->texture->width) / globalAnimatable->cellanim->textureW;
            float mismatchScaleY = static_cast<float>(globalAnimatable->texture->height) / globalAnimatable->cellanim->textureH;

            ImVec2 topLeftOffset = {
                (sourceRect[0] * scale * mismatchScaleX) + imagePosition.x,
                (sourceRect[1] * scale * mismatchScaleY) + imagePosition.y,
            };

            ImVec2 bottomRightOffset = {
                topLeftOffset.x + (sourceRect[2] * scale * mismatchScaleX),
                topLeftOffset.y + (sourceRect[3] * scale * mismatchScaleY)
            };

            drawList->AddRect(topLeftOffset, bottomRightOffset, IM_COL32(255, 0, 0, 255));
        }
    }

    ImGui::End();

    // Update Palette Window
    this->PaletteWindow();
}
