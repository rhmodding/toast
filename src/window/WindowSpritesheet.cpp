#include "WindowSpritesheet.hpp"

#include <imgui.h>

#include <cstdlib>

#include <iostream>
#include <sstream>

#include <set>
#include <vector>

#include <string>

#include <filesystem>

#include <thread>

#include <tinyfiledialogs.h>

#include "../font/FontAwesome.h"

#include "../AppState.hpp"

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"
#include "../MtCommandManager.hpp"

#include "../texture/ImageConvert.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "../stb/stb_rect_pack.h"

bool operator==(const stbrp_rect& lhs, const stbrp_rect& rhs) {
    return lhs.w == rhs.w &&
           lhs.h == rhs.h &&
           lhs.x == rhs.x &&
           lhs.y == rhs.y;
}

#define SHEET_ZOOM_TIME (.3f) // seconds

void WindowSpritesheet::RunEditor() {
    GET_CONFIG_MANAGER;

    // Some apps like Photoshop complain about relative paths
    std::string imagePath = std::filesystem::absolute(configManager.getConfig().textureEditPath.c_str()).string();

    std::ostringstream commandStream;

#ifdef __WIN32__
    commandStream <<
        "cmd.exe /c \"\"" <<
        configManager.getConfig().imageEditorPath <<
        "\" \"" << imagePath << "\"\"";
#else
    commandStream <<
        "\"" <<
        configManager.getConfig().imageEditorPath <<
        "\" \"" << imagePath << "\"";
#endif

    std::string command = commandStream.str();
    std::cout << "[WindowSpritesheet::RunEditor] Running command: " << command << std::endl;

    std::thread t([command]() {
        std::system(command.c_str());
    }); t.detach();
}

void WindowSpritesheet::FormatPopup() {
    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);

    CENTER_NEXT_WINDOW;

    ImGui::SetNextWindowSize({ 800.f, 550.f }, ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Re-encode image..###ReencodePopup", nullptr)) {
        GET_SESSION_MANAGER;

        auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

        static const char* formatList[] {
            "RGBA32",
            "RGB5A3",
        };
        static int selectedFormatIndex { 0 };

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

            ImGui::SetNextWindowSizeConstraints(
                { 270.f, 0.f },
                {
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()
                }
            );
            ImGui::BeginChild(
                "Properties",
                { 0.f, -ImGui::GetFrameHeightWithSpacing() },
                ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX
            );

            {
                ImGui::PushFont(AppState::getInstance().fonts.large);
                ImGui::SeparatorText("Image Info");
                ImGui::PopFont();

                if (ImGui::BeginChild(
                    "ImageInfoChild",
                    { 0.f, 0.f },
                    ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY
                )) {
                    unsigned imageWidth = cellanimSheet->getWidth();
                    unsigned imageHeight = cellanimSheet->getHeight();

                    ImGui::BulletText("Size: %ux%u", imageWidth, imageHeight);

                    uint32_t dataSize = ImageConvert::getImageByteSize(tplFormat, imageWidth, imageHeight);

                    char formattedStr[32];
                    {
                        char numberStr[32];
                        snprintf(numberStr, sizeof(numberStr), "%u", dataSize);

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

                if (ImGui::BeginChild(
                    "FormatInfoChild",
                    { 0.f, 0.f },
                    ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY
                )) {
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
                cellanimSheet->setTPLOutputFormat(tplFormat);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndGroup();
        }

        ImGui::SameLine();

        // Right
        {
            ImGui::SetNextWindowSizeConstraints(
                { 256.f, 0.f },
                {
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()
                }
            );
            ImGui::BeginChild("ImageView");

            static float bgScale { .215f };

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##BGSlider", &bgScale, 0.f, 1.f, "BG: %.3f");

            ImVec2 imageRect {
                static_cast<float>(cellanimSheet->getWidth()),
                static_cast<float>(cellanimSheet->getHeight())
            };

            float scale;
            Common::FitRect(imageRect, ImGui::GetContentRegionAvail(), scale);

            ImVec2 imagePosition {
                ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - imageRect.x) * .5f,
                ImGui::GetCursorScreenPos().y + (ImGui::GetContentRegionAvail().y - imageRect.y) * .5f
            };

            ImGui::GetWindowDrawList()->AddRectFilled(
                imagePosition,
                { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, },
                ImGui::ColorConvertFloat4ToU32({ bgScale, bgScale, bgScale, 1.f })
            );

            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)cellanimSheet->getTextureId(),
                imagePosition,
                { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
            );

            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

/* TODO: reimplement with undo support
void RepackSheet() {
    const unsigned padding = 4;
    const unsigned paddingHalf = padding / 2;

    const uint32_t borderCol = 0xFF000000;

    GET_SESSION_MANAGER;

    auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

    const auto& cellanimObject = sessionManager.getCurrentSession()->getCellanimObject();
    auto arrangements = cellanimObject->arrangements; // Create copy instead of reference, move when finished

    struct RectComparator {
        bool operator()(const stbrp_rect& lhs, const stbrp_rect& rhs) const {
            if (lhs.w != rhs.w) return lhs.w < rhs.w;
            if (lhs.h != rhs.h) return lhs.h < rhs.h;
            if (lhs.x != rhs.x) return lhs.x < rhs.x;
            return lhs.y < rhs.y;
        }
    };
    std::set<stbrp_rect, RectComparator> _rectsSet;

    for (const auto& arrangement : arrangements) {
        for (const auto& part : arrangement.parts) {
            stbrp_rect rect;
            rect.w = part.regionW + padding;
            rect.h = part.regionH + padding;
            rect.x = part.regionX - paddingHalf;
            rect.y = part.regionY - paddingHalf;

            _rectsSet.insert(rect);
        }
    }

    const unsigned numRects = _rectsSet.size();

    std::vector<stbrp_rect> rects(_rectsSet.begin(), _rectsSet.end());
    std::vector<stbrp_rect> originalRects = rects;

    // _rectsSet not used from here on out
    _rectsSet.clear();

    // Internal storage for stbrp
    std::vector<stbrp_node> nodes(numRects);

    stbrp_context context;
    stbrp_init_target(&context, cellanimSheet->width, cellanimSheet->height, nodes.data(), numRects);

    int successful = stbrp_pack_rects(&context, rects.data(), numRects);
    if (!successful)
        return;

    unsigned char* newImage = new unsigned char[cellanimSheet->width * cellanimSheet->height * 4];
    memset(newImage, 0, cellanimSheet->width * cellanimSheet->height * 4);

    unsigned char* srcImage = new unsigned char[cellanimSheet->width * cellanimSheet->height * 4];

    // Copy cellanim sheet to srcImage
    std::future<void> futureA = MtCommandManager::getInstance().enqueueCommand([&cellanimSheet, srcImage]() {
        glBindTexture(GL_TEXTURE_2D, cellanimSheet->texture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, srcImage);
        glBindTexture(GL_TEXTURE_2D, 0);
    });
    futureA.get();

    // Create new image data
    for (unsigned i = 0; i < numRects; ++i) {
        stbrp_rect* rect = &rects[i];
        stbrp_rect* originalRect = &originalRects[i];

        int ox = originalRect->x;
        int oy = originalRect->y;
        
        int nx = rect->x;
        int ny = rect->y;

        int w = originalRect->w;
        int h = originalRect->h;

        // Copy the image rect
        for (unsigned row = 0; row < h; row++) {
            unsigned char* src = srcImage + ((oy + row) * cellanimSheet->width + ox) * 4;
            unsigned char* dst = newImage + ((ny + row) * cellanimSheet->width + nx) * 4;

            memcpy(dst, src, w * 4);
        }

        // Vertical borders
        for (unsigned col = 0; col < w; col++) {
            ((uint32_t*)newImage)[(ny * cellanimSheet->width) + nx + col] = borderCol;
            ((uint32_t*)newImage)[((ny + h - 1) * cellanimSheet->width) + nx + col] = borderCol;
        }

        // Horizontal borders
        for (unsigned row = 0; row < h; row++) {
            ((uint32_t*)newImage)[((ny + row) * cellanimSheet->width) + nx] = borderCol;
            ((uint32_t*)newImage)[((ny + row) * cellanimSheet->width) + nx + w - 1] = borderCol;
        }
    }

    // Assign new rects
    for (auto& arrangement : arrangements) {
        for (auto& part : arrangement.parts) {
            stbrp_rect rect;
            rect.w = part.regionW + padding;
            rect.h = part.regionH + padding;
            rect.x = part.regionX - paddingHalf;
            rect.y = part.regionY - paddingHalf;

            auto findRect = std::find(originalRects.begin(), originalRects.end(), rect);
            if (findRect != originalRects.end()) {
                stbrp_rect& newRect = rects[findRect - originalRects.begin()];

                part.regionW = newRect.w - padding;
                part.regionH = newRect.h - padding;
                part.regionX = newRect.x + paddingHalf;
                part.regionY = newRect.y + paddingHalf;
            }
        }
    }

    // Copy new image to cellanim sheet & move new arrangements
    std::future<void> futureB = MtCommandManager::getInstance().enqueueCommand([&cellanimSheet, &newImage, &cellanimObject, &arrangements]() {
        glBindTexture(GL_TEXTURE_2D, cellanimSheet->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cellanimSheet->width, cellanimSheet->height, GL_RGBA, GL_UNSIGNED_BYTE, newImage);
        glBindTexture(GL_TEXTURE_2D, 0);

        cellanimObject->arrangements = std::move(arrangements);
    });
    futureB.get();

    delete[] srcImage;
    delete[] newImage;
}
*/

void WindowSpritesheet::Update() {
    static bool firstOpen { true };
    if (firstOpen) {
        this->gridType = AppState::getInstance().getDarkThemeEnabled() ?
            GridType_Dark : GridType_Light;

        firstOpen = false;
    }

    GET_SESSION_MANAGER;

    auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
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

                if (ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&customGridColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                ))
                    gridType = GridType_Custom;

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
                    ImGui::OpenPopup("###WaitForModifiedTexture");
                    ImGui::PopID();

                    this->RunEditor();
                }
                else {
                    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                    ImGui::OpenPopup("###TextureExportFailed");
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

                    std::shared_ptr<Texture> newImage = std::make_shared<Texture>();
                    bool ok = newImage->LoadSTBFile(openFileDialog);

                    if (ok) {
                        auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

                        bool diffSize =
                            newImage->getWidth()  != cellanimSheet->getWidth() ||
                            newImage->getHeight() != cellanimSheet->getHeight();

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

            if (ImGui::MenuItem("Export image to file...", nullptr, false)) {
                const char* filterPatterns[] = { "*.png" };
                char* saveFileDialog = tinyfd_saveFileDialog(
                    "Select a file to save the image to",
                    nullptr,
                    ARRAY_LENGTH(filterPatterns), filterPatterns,
                    "Image file (.png)"
                );

                if (saveFileDialog) {
                    GET_SESSION_MANAGER;

                    sessionManager.getCurrentSession()->getCellanimSheet()
                        ->ExportToFile(saveFileDialog);
                }
            }

            if (ImGui::MenuItem((char*)ICON_FA_STAR " Re-pack sheet", nullptr, false)) {
                //RepackSheet();
                std::cerr << "Not implemented (repack sheet)\n";
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Data")) {
            auto imageFormat =
                sessionManager.getCurrentSession()->getCellanimSheet()->getTPLOutputFormat();

            ImGui::Text("Image Format: %s", TPL::getImageFormatName(imageFormat));

            ImGui::Separator();

            if (ImGui::MenuItem("Re-encode (change format)..")) {
                ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                ImGui::OpenPopup("###ReencodePopup");
                ImGui::PopID();
            }

            ImGui::EndMenu();
        }

        {
            char textBuffer[32];
            snprintf(
                textBuffer, sizeof(textBuffer),
                "Double-click to %s", this->sheetZoomEnabled ? "un-zoom" : "zoom"
            );

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
    ImVec2 canvasBottomRight { canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y };

    uint32_t backgroundColor { 0xFF000000 };
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

    ImVec2 imageRect {
        static_cast<float>(cellanimSheet->getWidth()),
        static_cast<float>(cellanimSheet->getHeight())
    };

    float scale;
    Common::FitRect(imageRect, canvasSize, scale);

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
    }
    else {
        float rScale = this->sheetZoomEnabled ? 2.f : 1.f;

        scale *= rScale;
        imageRect.x *= rScale;
        imageRect.y *= rScale;

        canvasOffset.x = this->sheetZoomEnabled ? this->sheetZoomedOffset.x : 0.f;
        canvasOffset.y = this->sheetZoomEnabled ? this->sheetZoomedOffset.y : 0.f;
    }

    ImVec2 imagePosition {
        (canvasTopLeft.x + (canvasSize.x - imageRect.x) * .5f) + canvasOffset.x,
        (canvasTopLeft.y + (canvasSize.y - imageRect.y) * .5f) + canvasOffset.y
    };

    drawList->AddImage(
        (ImTextureID)cellanimSheet->getTextureId(),
        imagePosition,
        { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
    );

    GET_APP_STATE;

    if (this->drawBounding) {
        GET_ANIMATABLE;

        RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable.getCurrentArrangement();

        for (unsigned i = 0; i < arrangementPtr->parts.size(); i++) {
            if (appState.focusOnSelectedPart && !appState.isPartSelected(i))
                continue;

            RvlCellAnim::ArrangementPart& part = arrangementPtr->parts[i];

            uint16_t sourceRect[4] {
                part.regionX, part.regionY, part.regionW, part.regionH
            };

            float mismatchScaleX =
                static_cast<float>(globalAnimatable.texture->getWidth()) / globalAnimatable.cellanim->textureW;
            float mismatchScaleY =
                static_cast<float>(globalAnimatable.texture->getHeight()) / globalAnimatable.cellanim->textureH;

            ImVec2 topLeftOffset {
                (sourceRect[0] * scale * mismatchScaleX) + imagePosition.x,
                (sourceRect[1] * scale * mismatchScaleY) + imagePosition.y,
            };

            ImVec2 bottomRightOffset {
                topLeftOffset.x + (sourceRect[2] * scale * mismatchScaleX),
                topLeftOffset.y + (sourceRect[3] * scale * mismatchScaleY)
            };

            drawList->AddRect(topLeftOffset, bottomRightOffset, IM_COL32(255, 0, 0, 255));
        }
    }

    ImGui::End();
}
