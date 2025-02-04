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

#include "../texture/ImageConvert.hpp"

#include "../command/CommandModifySpritesheet.hpp"
#include "../command/CommandModifyArrangements.hpp"

#include "../App/Popups.hpp"

#include "../Easings.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "../stb/stb_rect_pack.h"

bool operator==(const stbrp_rect& lhs, const stbrp_rect& rhs) {
    return lhs.w == rhs.w &&
           lhs.h == rhs.h &&
           lhs.x == rhs.x &&
           lhs.y == rhs.y;
}

static void fitRect(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale) {
    float widthRatio = targetRect.x / rectToFit.x;
    float heightRatio = targetRect.y / rectToFit.y;

    scale = std::min(widthRatio, heightRatio);

    rectToFit.x *= scale;
    rectToFit.y *= scale;
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
#endif // __WIN32__

    std::string command = commandStream.str();
    std::cout << "[WindowSpritesheet::RunEditor] Running command: " << command << std::endl;

    std::thread([command]() {
        std::system(command.c_str());
    }).detach();
}

void WindowSpritesheet::FormatPopup() {
    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);

    CENTER_NEXT_WINDOW;

    ImGui::SetNextWindowSize({ 800.f, 550.f }, ImGuiCond_Appearing);

    static bool lateOpen { false };
    bool open = ImGui::BeginPopupModal("Re-encode image..###ReencodePopup", nullptr);

    if (open) {
        GET_SESSION_MANAGER;

        std::shared_ptr cellanimSheet =
            sessionManager.getCurrentSession()->getCurrentCellanimSheet();

        constexpr const char* formats = "RGBA32\0RGB5A3\0";
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

        auto updateTextureData = [&]() {
            // Re-evaluate
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

            unsigned char* imageData = cellanimSheet->GetRGBA32();
            if (!imageData)
                return;

            unsigned bufferSize = ImageConvert::getImageByteSize(
                tplFormat, cellanimSheet->getWidth(), cellanimSheet->getHeight()
            );
            unsigned char* imageBuffer = new unsigned char[bufferSize];

            ImageConvert::fromRGBA32(
                imageBuffer, nullptr, nullptr, tplFormat,
                cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                imageData
            );

            // Swap buffers.
            ImageConvert::toRGBA32(
                imageData, tplFormat,
                cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                imageBuffer, nullptr
            );

            delete[] imageBuffer;

            this->formatPreviewTexture->LoadRGBA32(
                imageData,
                cellanimSheet->getWidth(), cellanimSheet->getHeight()
            );
            this->formatPreviewTexture->setTPLOutputFormat(tplFormat);

            delete[] imageData;
        };

        if (!lateOpen) {
            this->formatPreviewTexture = std::make_shared<Texture>();

            unsigned char* imageData = cellanimSheet->GetRGBA32();
            if (imageData) {
                this->formatPreviewTexture->LoadRGBA32(
                    imageData,
                    cellanimSheet->getWidth(), cellanimSheet->getHeight()
                );
                delete[] imageData;

                switch (cellanimSheet->getTPLOutputFormat()) {
                case TPL::TPL_IMAGE_FORMAT_RGBA32:
                    selectedFormatIndex = 0;
                    break;
                case TPL::TPL_IMAGE_FORMAT_RGB5A3:
                    selectedFormatIndex = 1;
                    break;
                
                default:
                    break;
                }

                updateTextureData();
            }
        }
        lateOpen = true;

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

                        unsigned srcLen = strlen(numberStr);
                        unsigned destLen = srcLen + (srcLen - 1) / 3;

                        int srcIndex = srcLen - 1;
                        int destIndex = destLen - 1;

                        unsigned digitCount = 0;

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
                        colorDesc =
                            "Color:\n"
                            "   Opaque pixels:\n"
                            "      15-bit, 32768 colors\n"
                            "   Transparent pixels:\n"
                            "      12-bit, 4096 colors";
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

            ImGui::Dummy({ 0.f, 5.f });
            ImGui::Separator();
            ImGui::Dummy({ 0.f, 5.f });

            if (ImGui::Combo("Format", &selectedFormatIndex, formats)) {
                updateTextureData();
            }

            ImGui::EndChild();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();

            if (ImGui::Button("Apply")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifySpritesheet>(
                    sessionManager.getCurrentSession()
                        ->getCurrentCellanim().object->sheetIndex,
                    this->formatPreviewTexture
                ));

                this->formatPreviewTexture.reset();

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
            fitRect(imageRect, ImGui::GetContentRegionAvail(), scale);

            ImVec2 imagePosition {
                ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - imageRect.x) * .5f,
                ImGui::GetCursorScreenPos().y + (ImGui::GetContentRegionAvail().y - imageRect.y) * .5f
            };

            ImGui::GetWindowDrawList()->AddRectFilled(
                imagePosition,
                { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, },
                ImGui::ColorConvertFloat4ToU32({ bgScale, bgScale, bgScale, 1.f })
            );

            if (this->formatPreviewTexture)
                ImGui::GetWindowDrawList()->AddImage(
                    (ImTextureID)this->formatPreviewTexture->getTextureId(),
                    imagePosition,
                    { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
                );

            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }
    else
        lateOpen = false;

    ImGui::PopID();
}

// TODO: move this somewhere else
bool RepackSheet() {
    constexpr int PADDING = 4;
    constexpr int PADDING_HALF = PADDING / 2;
    constexpr uint32_t BORDER_COLOR = 0xFF000000; // Black

    GET_SESSION_MANAGER;

    auto& session = *sessionManager.getCurrentSession();
    std::shared_ptr cellanimSheet = session.getCurrentCellanimSheet();
    std::shared_ptr cellanimObject = session.getCurrentCellanim().object;

    auto arrangements = cellanimObject->arrangements; // Copy

    struct RectComparator {
        bool operator()(const stbrp_rect& lhs, const stbrp_rect& rhs) const {
            return std::tie(lhs.w, lhs.h, lhs.x, lhs.y) < std::tie(rhs.w, rhs.h, rhs.x, rhs.y);
        }
    };

    std::set<stbrp_rect, RectComparator> uniqueRects;

    for (const auto& arrangement : arrangements) {
        for (const auto& part : arrangement.parts) {
            stbrp_rect rect = {
                .w = (int)part.regionW + PADDING,
                .h = (int)part.regionH + PADDING,
                .x = (int)part.regionX - PADDING_HALF,
                .y = (int)part.regionY - PADDING_HALF,
            };
            uniqueRects.insert(rect);
        }
    }

    const unsigned numRects = uniqueRects.size();
    std::vector<stbrp_rect> sortedRects(uniqueRects.begin(), uniqueRects.end());
    auto originalRects = sortedRects;

    // Initialize packing context
    std::vector<stbrp_node> nodes(numRects);
    stbrp_context context;
    stbrp_init_target(&context, cellanimSheet->getWidth(), cellanimSheet->getHeight(), nodes.data(), numRects);

    if (!stbrp_pack_rects(&context, sortedRects.data(), numRects))
        return false;

    const unsigned pixelCount = cellanimSheet->getPixelCount();
    std::unique_ptr<unsigned char[]> newImage(new unsigned char[pixelCount * 4]());
    std::unique_ptr<unsigned char[]> srcImage(new unsigned char[pixelCount * 4]);
    cellanimSheet->GetRGBA32(srcImage.get());

    // Copy image data to the new layout
    for (unsigned i = 0; i < numRects; ++i) {
        const auto& rect = sortedRects[i];
        const auto& origRect = originalRects[i];

        const int ox = origRect.x, oy = origRect.y, nx = rect.x, ny = rect.y;
        const int w = origRect.w, h = origRect.h;

        for (int row = 0; row < h; ++row) {
            auto* src = srcImage.get() + ((oy + row) * cellanimSheet->getWidth() + ox) * 4;
            auto* dst = newImage.get() + ((ny + row) * cellanimSheet->getWidth() + nx) * 4;
            memcpy(dst, src, w * 4);
        }

        // Add borders in one loop
        for (int col = 0; col < w; ++col) {
            ((uint32_t*)newImage.get())[ny * cellanimSheet->getWidth() + nx + col] = BORDER_COLOR;
            ((uint32_t*)newImage.get())[(ny + h - 1) * cellanimSheet->getWidth() + nx + col] = BORDER_COLOR;
        }
        for (int row = 0; row < h; ++row) {
            ((uint32_t*)newImage.get())[(ny + row) * cellanimSheet->getWidth() + nx] = BORDER_COLOR;
            ((uint32_t*)newImage.get())[(ny + row) * cellanimSheet->getWidth() + nx + w - 1] = BORDER_COLOR;
        }
    }

    // Update texture
    auto newTexture = std::make_shared<Texture>();
    newTexture->LoadRGBA32(newImage.get(), cellanimSheet->getWidth(), cellanimSheet->getHeight());

    // Update arrangements
    for (auto& arrangement : arrangements) {
        for (auto& part : arrangement.parts) {
            stbrp_rect rect = {
                .w = (int)part.regionW + PADDING,
                .h = (int)part.regionH + PADDING,
                .x = (int)part.regionX - PADDING_HALF,
                .y = (int)part.regionY - PADDING_HALF,
            };

            auto it = std::find(originalRects.begin(), originalRects.end(), rect);
            if (it != originalRects.end()) {
                const auto& newRect = sortedRects[std::distance(originalRects.begin(), it)];
                part.regionW = newRect.w - PADDING;
                part.regionH = newRect.h - PADDING;
                part.regionX = newRect.x + PADDING_HALF;
                part.regionY = newRect.y + PADDING_HALF;
            }
        }
    }

    session.addCommand(std::make_shared<CommandModifySpritesheet>(cellanimObject->sheetIndex, newTexture));
    session.addCommand(std::make_shared<CommandModifyArrangements>(session.currentCellanim, std::move(arrangements)));

    return true;
}

void WindowSpritesheet::Update() {
    static bool firstOpen { true };
    if (firstOpen) {
        this->gridType = AppState::getInstance().getDarkThemeEnabled() ?
            GridType_Dark : GridType_Light;

        firstOpen = false;
    }

    GET_SESSION_MANAGER;

    std::shared_ptr cellanimSheet = sessionManager.getCurrentSession()->getCurrentCellanimSheet();

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

                if (cellanimSheet->ExportToFile(configManager.getConfig().textureEditPath.c_str())) {
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

                    std::shared_ptr<Texture> newTexture = std::make_shared<Texture>();
                    bool ok = newTexture->LoadSTBFile(openFileDialog);

                    if (ok) {
                        bool diffSize =
                            newTexture->getWidth()  != cellanimSheet->getWidth() ||
                            newTexture->getHeight() != cellanimSheet->getHeight();

                        Popups::_oldTextureSizeX = cellanimSheet->getWidth();
                        Popups::_oldTextureSizeY = cellanimSheet->getHeight();

                        sessionManager.getCurrentSession()->addCommand(
                        std::make_shared<CommandModifySpritesheet>(
                            sessionManager.getCurrentSession()
                                ->getCurrentCellanim().object->sheetIndex,
                            newTexture
                        ));

                        if (diffSize) {
                            ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                            ImGui::OpenPopup("###ModifiedTextureSize");
                            ImGui::PopID();
                        }
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

                if (saveFileDialog)
                    cellanimSheet->ExportToFile(saveFileDialog);
            }

            if (ImGui::MenuItem((const char*)ICON_FA_STAR " Re-pack sheet", nullptr, false)) {
                bool ok = RepackSheet();
                if (!ok)
                    AppState::getInstance().OpenGlobalPopup("###SheetRepackFailed");
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Data")) {
            auto imageFormat =
                sessionManager.getCurrentSession()
                    ->getCurrentCellanimSheet()->getTPLOutputFormat();

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

    uint32_t backgroundColor { 0xFF000000 }; // Black
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
    fitRect(imageRect, canvasSize, scale);

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
        float rScale = (this->sheetZoomEnabled ? Easings::In : Easings::Out)(
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

            unsigned sourceRect[4] {
                part.regionX, part.regionY, part.regionW, part.regionH
            };

            float mismatchScaleX =
                static_cast<float>(globalAnimatable.texture->getWidth()) / globalAnimatable.cellanim->sheetW;
            float mismatchScaleY =
                static_cast<float>(globalAnimatable.texture->getHeight()) / globalAnimatable.cellanim->sheetH;

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
