#include "WindowSpritesheet.hpp"

#include <cstddef>

#include <cstdlib>

#include <sstream>

#include "Logging.hpp"

#include <set>
#include <vector>

#include <string>

#include <filesystem>

#include <thread>

#include <imgui.h>

#include <tinyfiledialogs.h>

#include "font/FontAwesome.h"

#include "manager/AppState.hpp"

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/ThemeManager.hpp"
#include "manager/PromptPopupManager.hpp"

#include "texture/TextureEx.hpp"

#include "texture/RvlImageConvert.hpp"
#include "texture/CtrImageConvert.hpp"

#include "command/CommandModifySpritesheet.hpp"
#include "command/CommandModifyArrangements.hpp"

#include "command/CompositeCommand.hpp"

#include "App/Popups.hpp"
#include "App/popups/ModifiedTextureSize.hpp"

#include "util/EaseUtil.hpp"

#include "util/SpritesheetFixUtil.hpp"

#include "stb/stb_rect_pack.h"

bool operator==(const stbrp_rect& lhs, const stbrp_rect& rhs) {
    return lhs.w == rhs.w &&
           lhs.h == rhs.h &&
           lhs.x == rhs.x &&
           lhs.y == rhs.y;
}

static float fitRect(ImVec2 &rectToFit, const ImVec2 &targetRect) {
    float widthRatio = targetRect.x / rectToFit.x;
    float heightRatio = targetRect.y / rectToFit.y;

    float scale = std::min(widthRatio, heightRatio);

    rectToFit.x *= scale;
    rectToFit.y *= scale;

    return scale;
}

constexpr float SHEET_ZOOM_TIME = .3f; // seconds

void WindowSpritesheet::RunEditor() {
    const Config& config = ConfigManager::getInstance().getConfig();

    // Some apps like Photoshop complain about relative paths
    std::string imagePath = std::filesystem::absolute(config.textureEditPath.c_str()).string();

    std::ostringstream commandStream;

#ifdef _WIN32
    commandStream <<
        "cmd.exe /c \"\"" <<
        config.imageEditorPath <<
        "\" \"" << imagePath << "\"\"";
#else
    commandStream <<
        "\"" <<
        config.imageEditorPath <<
        "\" \"" << imagePath << "\"";
#endif // _WIN32

    std::string command = commandStream.str();
    Logging::info << "[WindowSpritesheet::RunEditor] Running command: " << command << std::endl;

    std::thread([command]() {
        std::system(command.c_str());
    }).detach();
}

void WindowSpritesheet::FormatPopup() {
    BEGIN_GLOBAL_POPUP();

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    ImGui::SetNextWindowSize({ 800.f, 550.f }, ImGuiCond_Appearing);

    static bool lateOpen { false };
    bool open = ImGui::BeginPopupModal("Re-encode image..###ReencodePopup", nullptr);

    if (open) {
        SessionManager& sessionManager = SessionManager::getInstance();

        std::shared_ptr cellanimSheet =
            sessionManager.getCurrentSession()->getCurrentCellAnimSheet();

        const bool isRVL = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL;

        constexpr std::array<TPL::TPLImageFormat, 4> rvlFormats = {
            TPL::TPL_IMAGE_FORMAT_RGBA32,
            TPL::TPL_IMAGE_FORMAT_RGB5A3,
            TPL::TPL_IMAGE_FORMAT_CMPR,
            TPL::TPL_IMAGE_FORMAT_C14X2
        };
        constexpr unsigned defaultRvlFormatIdx = 1;

        constexpr std::array<CTPK::CTPKImageFormat, 3> ctrFormats = {
            CTPK::CTPK_IMAGE_FORMAT_RGBA8888,
            CTPK::CTPK_IMAGE_FORMAT_RGBA4444,
            CTPK::CTPK_IMAGE_FORMAT_ETC1A4
        };
        constexpr unsigned defaultCtrFormatIdx = 2;

        constexpr std::array rvlFormatNames = [&rvlFormats] {
            std::array<const char*, rvlFormats.size()> names {};
            for (size_t i = 0; i < rvlFormats.size(); i++)
                names[i] = TPL::getImageFormatName(rvlFormats[i]);
            return names;
        }();
        constexpr std::array ctrFormatNames = [&ctrFormats] {
            std::array<const char*, ctrFormats.size()> names {};
            for (size_t i = 0; i < ctrFormats.size(); i++)
                names[i] = CTPK::getImageFormatName(ctrFormats[i]);
            return names;
        }();

        static int selectedFormatIndex { 0 };
        static int mipCount { 1 };

        static unsigned colorPaletteCount { 0 };
        static unsigned colorPaletteSize { 0 };

        auto updateTextureData = [&]() {
            unsigned selectedMax = isRVL ? rvlFormats.size() - 1 : ctrFormats.size() - 1;
            if (selectedFormatIndex > selectedMax)
                selectedFormatIndex = selectedMax;

            unsigned char* imageData = cellanimSheet->GetRGBA32();
            if (!imageData)
                return;

            if (isRVL) {
                unsigned bufferSize = RvlImageConvert::getImageByteSize(
                    rvlFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight()
                );

                unsigned char* imageBuffer = new unsigned char[bufferSize];

                std::vector<uint32_t> colorPalette;
                colorPaletteCount = 0;

                // Only actually initialize color palette buffer if needed.
                switch (rvlFormats[selectedFormatIndex]) {
                case TPL::TPL_IMAGE_FORMAT_C14X2:
                    colorPalette = std::vector<uint32_t>(16384);
                    break;
                case TPL::TPL_IMAGE_FORMAT_C8:
                    colorPalette = std::vector<uint32_t>(256);
                    break;
                case TPL::TPL_IMAGE_FORMAT_C4:
                    colorPalette = std::vector<uint32_t>(16);
                    break;

                default:
                    break;
                }

                RvlImageConvert::fromRGBA32(
                    imageBuffer, colorPalette.data(), &colorPaletteCount,
                    rvlFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                    imageData
                );

                // colorPaletteCount * sizeof CLUT entry
                colorPaletteSize = colorPaletteCount * 2;

                // Swap buffers.
                RvlImageConvert::toRGBA32(
                    imageData, rvlFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                    imageBuffer, colorPalette.data()
                );

                delete[] imageBuffer;
            }
            else {
                unsigned bufferSize = CtrImageConvert::getImageByteSize(
                    ctrFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(), 1
                );

                unsigned char* imageBuffer = new unsigned char[bufferSize];

                CtrImageConvert::fromRGBA32(
                    imageBuffer, ctrFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                    imageData
                );

                // Swap buffers.
                CtrImageConvert::toRGBA32(
                    imageData, ctrFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                    imageBuffer
                );

                delete[] imageBuffer;
            }

            mFormattingNewTex->LoadRGBA32(
                imageData,
                cellanimSheet->getWidth(), cellanimSheet->getHeight()
            );

            if (isRVL)
                mFormattingNewTex->setTPLOutputFormat(rvlFormats[selectedFormatIndex]);
            else
                mFormattingNewTex->setCTPKOutputFormat(ctrFormats[selectedFormatIndex]);

            delete[] imageData;
        };

        if (!lateOpen) {
            mipCount = cellanimSheet->getOutputMipCount();

            mFormattingNewTex = std::make_shared<TextureEx>();

            unsigned char* imageData = cellanimSheet->GetRGBA32();
            if (imageData) {
                mFormattingNewTex->LoadRGBA32(
                    imageData,
                    cellanimSheet->getWidth(), cellanimSheet->getHeight()
                );
                delete[] imageData;

                mFormattingNewTex->setOutputMipCount(mipCount);

                if (isRVL) {
                    auto it = std::find(
                        rvlFormats.begin(), rvlFormats.end(), cellanimSheet->getTPLOutputFormat()
                    );

                    if (it != rvlFormats.end())
                        selectedFormatIndex = std::distance(rvlFormats.begin(), it);
                    else
                        selectedFormatIndex = defaultRvlFormatIdx;
                }
                else {
                    auto it = std::find(
                        ctrFormats.begin(), ctrFormats.end(), cellanimSheet->getCTPKOutputFormat()
                    );

                    if (it != ctrFormats.end())
                        selectedFormatIndex = std::distance(ctrFormats.begin(), it);
                    else
                        selectedFormatIndex = defaultCtrFormatIdx;
                }
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
                ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX
            );

            {
                ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.4f);
                ImGui::SeparatorText("Image Info");
                ImGui::PopFont();

                if (ImGui::BeginChild(
                    "ImageInfoChild",
                    { 0.f, 0.f },
                    ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY
                )) {
                    unsigned imageWidth = cellanimSheet->getWidth();
                    unsigned imageHeight = cellanimSheet->getHeight();

                    ImGui::BulletText("Size: %ux%u", imageWidth, imageHeight);

                    uint32_t dataSize;

                    if (isRVL) {
                        dataSize = RvlImageConvert::getImageByteSize(
                            rvlFormats[selectedFormatIndex], imageWidth, imageHeight
                        ) + colorPaletteSize;
                    }
                    else {
                        dataSize = CtrImageConvert::getImageByteSize(
                            ctrFormats[selectedFormatIndex], imageWidth, imageHeight, mipCount
                        );
                    }


                    char formattedStr[32];
                    {
                        char numberStr[32];
                        std::snprintf(numberStr, sizeof(numberStr), "%u", dataSize);

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
                        formattedStr[destLen] = '\0';
                    }

                    ImGui::BulletText("Data Size: %sB", formattedStr);
                }
                ImGui::EndChild();
            }
            {
                ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.4f);
                ImGui::SeparatorText("Format Info");
                ImGui::PopFont();

                if (ImGui::BeginChild(
                    "FormatInfoChild",
                    { 0.f, 0.f },
                    ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY
                )) {
                    const char* colorDesc = nullptr;
                    const char* alphaDesc = nullptr;
                    bool showPaletteCount = false;

                    if (isRVL) {
                        switch (rvlFormats[selectedFormatIndex]) {
                        case TPL::TPL_IMAGE_FORMAT_RGBA32:
                            colorDesc = "Colors: 24-bit, millions of colors";
                            alphaDesc = "Alpha: 8-bit, ranged 0 to 255";
                            break;
                        case TPL::TPL_IMAGE_FORMAT_C14X2: // Uses RGB5A3 for the CLUT.
                            showPaletteCount = true;
                        case TPL::TPL_IMAGE_FORMAT_RGB5A3:
                            colorDesc =
                                "Color:\n"
                                "   Opaque pixels:\n"
                                "      15-bit, 32768 colors\n"
                                "   Transparent pixels:\n"
                                "      12-bit, 4096 colors";
                            alphaDesc = "Alpha: 3-bit, ranged 0 to 7";
                            break;
                        case TPL::TPL_IMAGE_FORMAT_CMPR:
                            colorDesc =
                                "Color:\n"
                                "   Opaque pixels:\n"
                                "      15-bit, 32768 colors\n"
                                "   Transparent pixels:\n"
                                "      12-bit, 4096 colors";
                            alphaDesc = "Alpha: 3-bit, ranged 0 to 7";
                            break;
                        default:
                            break;
                        }
                    }
                    else {
                        switch (ctrFormats[selectedFormatIndex]) {
                        case CTPK::CTPK_IMAGE_FORMAT_RGBA8888:
                            colorDesc = "Colors: 24-bit, millions of colors";
                            alphaDesc = "Alpha: 8-bit, ranged 0 to 255";
                            break;
                        case CTPK::CTPK_IMAGE_FORMAT_RGBA4444:
                            colorDesc = "Colors: 12-bit, 4096 colors";
                            alphaDesc = "Alpha: 4-bit, ranged 0 to 15";
                            break;
                        case CTPK::CTPK_IMAGE_FORMAT_ETC1A4:
                            colorDesc = "Colors (per block): 24-bit, millions of colors";
                            alphaDesc = "Alpha: 4-bit, ranged 0 to 15";
                            break;
                        default:
                            break;
                        }
                    }

                    if (colorDesc)
                        ImGui::BulletText("%s", colorDesc);
                    if (alphaDesc)
                        ImGui::BulletText("%s", alphaDesc);
                    if (showPaletteCount)
                        ImGui::BulletText("Paletted (%u colors)", colorPaletteCount);
                }
                ImGui::EndChild();
            }

            ImGui::Dummy({ 0.f, 5.f });
            ImGui::Separator();
            ImGui::Dummy({ 0.f, 5.f });

            const char* const* comboNames = isRVL ? rvlFormatNames.data() : ctrFormatNames.data();
            unsigned comboCount = isRVL ? rvlFormats.size() : ctrFormats.size();

            if (ImGui::Combo("Format", &selectedFormatIndex, comboNames, comboCount)) {
                unsigned selectedMax = isRVL ? rvlFormats.size() - 1 : ctrFormats.size() - 1;
                if (selectedFormatIndex > selectedMax)
                    selectedFormatIndex = selectedMax;

                updateTextureData();
            }

            if (ImGui::InputInt("Mip-levels", &mipCount, 1, 2)) {
                if (mipCount < 1) mipCount = 1;
                if (mipCount > 8) mipCount = 8;

                mFormattingNewTex->setOutputMipCount(mipCount);
            }

            ImGui::EndChild();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();

            if (ImGui::Button("Apply")) {
                mFormattingNewTex->setName(cellanimSheet->getName());

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifySpritesheet>(
                    sessionManager.getCurrentSession()
                        ->getCurrentCellAnim().object->getSheetIndex(),
                    mFormattingNewTex
                ));

                mFormattingNewTex.reset();

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

            fitRect(imageRect, ImGui::GetContentRegionAvail());

            ImVec2 imagePosition {
                ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - imageRect.x) * .5f,
                ImGui::GetCursorScreenPos().y + (ImGui::GetContentRegionAvail().y - imageRect.y) * .5f
            };

            ImGui::GetWindowDrawList()->AddRectFilled(
                imagePosition,
                { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, },
                ImGui::ColorConvertFloat4ToU32({ bgScale, bgScale, bgScale, 1.f })
            );

            if (mFormattingNewTex)
                ImGui::GetWindowDrawList()->AddImage(
                    mFormattingNewTex->getImTextureId(),
                    imagePosition,
                    { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
                );

            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }
    else
        lateOpen = false;

    END_GLOBAL_POPUP();
}

void WindowSpritesheet::Update() {
    static bool firstOpen { true };
    if (firstOpen) {
        mGridType = ThemeManager::getInstance().getThemeIsLight() ?
            GridType_Light : GridType_Dark;

        firstOpen = false;
    }

    SessionManager& sessionManager = SessionManager::getInstance();

    std::shared_ptr cellanimSheet = sessionManager.getCurrentSession()->getCurrentCellAnimSheet();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::Begin("Spritesheet", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImGuiIO& io = ImGui::GetIO();

    const ImVec2 windowSize = ImGui::GetContentRegionAvail();

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Grid")) {
            if (ImGui::MenuItem("None", nullptr, mGridType == GridType_None))
                mGridType = GridType_None;

            if (ImGui::MenuItem("Dark", nullptr, mGridType == GridType_Dark))
                mGridType = GridType_Dark;

            if (ImGui::MenuItem("Light", nullptr, mGridType == GridType_Light))
                mGridType = GridType_Light;

            if (ImGui::BeginMenu("Custom")) {
                bool enabled = mGridType == GridType_Custom;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    if (enabled)
                        mGridType = GridType_Custom;
                    else
                        mGridType = ThemeManager::getInstance().getThemeIsLight() ? GridType_Light : GridType_Dark;
                }

                ImGui::SeparatorText("Color Picker");

                if (ImGui::ColorPicker4(
                    "##ColorPicker",
                    &mCustomGridColor.x,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                ))
                    mGridType = GridType_Custom;

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Draw bounding boxes of active part(s)", nullptr, mDrawBounding))
                mDrawBounding ^= true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            bool imageEditorDefined = !ConfigManager::getInstance().getConfig().imageEditorPath.empty();

            if (ImGui::MenuItem("Edit in editing tool...", nullptr, false, imageEditorDefined)) {
                const ConfigManager& configManager = ConfigManager::getInstance();

                if (cellanimSheet->ExportToFile(configManager.getConfig().textureEditPath.c_str())) {
                    OPEN_GLOBAL_POPUP("###WaitForModifiedTexture");
                    RunEditor();
                }
                else {
                    PromptPopupManager::getInstance().Queue(
                        PromptPopupManager::CreatePrompt(
                            "An error occurred while exporting the texture..",
                            "The texture could not be exported to the PNG file; please\n"
                            "check the log for more details."
                        )
                        .WithResponses(PromptPopup::RESPONSE_OK)
                    );
                }
            }
            if (ImGui::MenuItem("Replace with new image...", nullptr, false)) {
                const char* filterPatterns[] = { "*.png", "*.tga", ".psd", "*.bmp", "*.jpg" };
                char* openFileDialog = tinyfd_openFileDialog(
                    "Select a replacement image file",
                    nullptr,
                    ARRAY_LENGTH(filterPatterns), filterPatterns,
                    "Image file (.png, .tga, .psd, .bmp, .jpg)",
                    false
                );

                if (openFileDialog) {
                    SessionManager& sessionManager = SessionManager::getInstance();

                    std::shared_ptr<TextureEx> newTexture = std::make_shared<TextureEx>();
                    if (newTexture->LoadSTBFile(openFileDialog)) {
                        bool diffSize =
                            newTexture->getWidth()  != cellanimSheet->getWidth() ||
                            newTexture->getHeight() != cellanimSheet->getHeight();

                        Popups::ModifiedTextureSize::getInstance().setOldTextureSizes(cellanimSheet->getWidth(), cellanimSheet->getHeight());

                        newTexture->setName(cellanimSheet->getName());

                        sessionManager.getCurrentSession()->addCommand(
                        std::make_shared<CommandModifySpritesheet>(
                            sessionManager.getCurrentSession()
                                ->getCurrentCellAnim().object->getSheetIndex(),
                            newTexture
                        ));

                        if (diffSize) {
                            OPEN_GLOBAL_POPUP("###ModifiedTextureSize");
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
                if (!SpritesheetFixUtil::FixRepack(*sessionManager.getCurrentSession()))
                    OPEN_GLOBAL_POPUP("###SheetRepackFailed");
            }

            if (ImGui::MenuItem((const char*)ICON_FA_STAR " Fix sheet alpha bleeding", nullptr, false)) {
                SpritesheetFixUtil::FixAlphaBleed(*sessionManager.getCurrentSession());
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Data")) {
            const char* imageFormatName;

            if (sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL)
                imageFormatName = TPL::getImageFormatName(cellanimSheet->getTPLOutputFormat());
            else
                imageFormatName = CTPK::getImageFormatName(cellanimSheet->getCTPKOutputFormat());

            ImGui::Text("Image Format: %s", imageFormatName);

            ImGui::Separator();

            if (ImGui::MenuItem("Re-encode (change format)..")) {
                OPEN_GLOBAL_POPUP("###ReencodePopup");
            }

            ImGui::EndMenu();
        }

        {
            char textBuffer[32];
            std::snprintf(
                textBuffer, sizeof(textBuffer),
                "Double-click to %s", mSheetZoomEnabled ? "un-zoom" : "zoom"
            );

            ImGui::SetCursorPosX(
                windowSize.x - ImGui::CalcTextSize(textBuffer).x -
                ImGui::GetStyle().FramePadding.x
            );
            ImGui::TextUnformatted(textBuffer);
        }

        ImGui::EndMenuBar();
    }

    FormatPopup();

    ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasBottomRight { canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y };

    uint32_t backgroundColor { IM_COL32(0,0,0,0xFF) };
    switch (mGridType) {
    case GridType_None:
        backgroundColor = IM_COL32(0,0,0,0);
        break;

    case GridType_Dark:
        backgroundColor = IM_COL32(50, 50, 50, 255);
        break;

    case GridType_Light:
        backgroundColor = IM_COL32(255, 255, 255, 255);
        break;

    case GridType_Custom:
        backgroundColor = IM_COL32(
            mCustomGridColor.x * 255,
            mCustomGridColor.y * 255,
            mCustomGridColor.z * 255,
            mCustomGridColor.w * 255
        );
        break;

    default:
        break;
    }

    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions
    if (canvasSize.x != 0.f && canvasSize.y != 0.f)
        ImGui::InvisibleButton("CanvasInteractions", canvasSize,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonRight |
            ImGuiButtonFlags_MouseButtonMiddle
        );

    const bool interactionHovered = ImGui::IsItemHovered();
    const bool interactionActive = ImGui::IsItemActive(); // Held

    // Inital sheet zoom
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && interactionHovered) {
        mSheetZoomTriggered = true;
        mSheetZoomTimer = static_cast<float>(ImGui::GetTime());
    }

    if (
        mSheetZoomTriggered &&
        (
            static_cast<float>(ImGui::GetTime()) -
            mSheetZoomTimer
        ) > SHEET_ZOOM_TIME
    ) {
        mSheetZoomEnabled = !mSheetZoomEnabled;
        mSheetZoomTriggered = false;
    }

    // Dragging
    const float mousePanThreshold = -1.f;
    bool draggingCanvas = mSheetZoomEnabled && interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold)
    );

    ImVec2 imageRect {
        static_cast<float>(cellanimSheet->getWidth()),
        static_cast<float>(cellanimSheet->getHeight())
    };

    float scale = fitRect(imageRect, canvasSize);

    if (draggingCanvas) {
        mSheetZoomedOffset.x += io.MouseDelta.x;
        mSheetZoomedOffset.y += io.MouseDelta.y;

        mSheetZoomedOffset.x = std::clamp<float>(mSheetZoomedOffset.x, -(imageRect.x / 2), imageRect.x / 2);
        mSheetZoomedOffset.y = std::clamp<float>(mSheetZoomedOffset.y, -(imageRect.y / 2), imageRect.y / 2);
    }

    ImVec2 canvasOffset;

    if (mSheetZoomTriggered) {
        float rel = (ImGui::GetTime() - mSheetZoomTimer) / SHEET_ZOOM_TIME;
        float rScale = (mSheetZoomEnabled ? EaseUtil::In : EaseUtil::Out)(
            mSheetZoomEnabled ? 1.f - rel : rel
        ) + 1.f;

        scale *= rScale;
        imageRect.x *= rScale;
        imageRect.y *= rScale;

        canvasOffset.x = mSheetZoomedOffset.x * (rScale - 1.f);
        canvasOffset.y = mSheetZoomedOffset.y * (rScale - 1.f);
    }
    else {
        float rScale = mSheetZoomEnabled ? 2.f : 1.f;

        scale *= rScale;
        imageRect.x *= rScale;
        imageRect.y *= rScale;

        canvasOffset.x = mSheetZoomEnabled ? mSheetZoomedOffset.x : 0.f;
        canvasOffset.y = mSheetZoomEnabled ? mSheetZoomedOffset.y : 0.f;
    }

    ImVec2 imagePosition {
        (canvasTopLeft.x + (canvasSize.x - imageRect.x) * .5f) + canvasOffset.x,
        (canvasTopLeft.y + (canvasSize.y - imageRect.y) * .5f) + canvasOffset.y
    };

    drawList->AddImage(
        cellanimSheet->getImTextureId(),
        imagePosition,
        { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
    );

    AppState& appState = AppState::getInstance();

    if (mDrawBounding) {
        PlayerManager& playerManager = PlayerManager::getInstance();

        const auto& session = sessionManager.getCurrentSession();

        const auto& textureGroup = session->sheets;
        const std::shared_ptr cellanimObject = session->getCurrentCellAnim().object;

        const auto& arrangement = playerManager.getArrangement();

        for (size_t i = 0; i < arrangement.parts.size(); i++) {
            const auto& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

            if (appState.mFocusOnSelectedPart && !selectionState.isPartSelected(i))
                continue;

            const auto& part = arrangement.parts[i];

            unsigned sourceRect[4] {
                part.cellOrigin.x % cellanimObject->getSheetWidth(),
                part.cellOrigin.y % cellanimObject->getSheetHeight(),
                part.cellSize.x, part.cellSize.y
            };

            const auto& associatedTex = textureGroup->getTextureByVarying(part.textureVarying);

            float mismatchScaleX =
                static_cast<float>(associatedTex->getWidth()) / cellanimObject->getSheetWidth();
            float mismatchScaleY =
                static_cast<float>(associatedTex->getHeight()) / cellanimObject->getSheetHeight();

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
