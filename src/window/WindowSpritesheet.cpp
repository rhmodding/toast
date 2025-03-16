#include "WindowSpritesheet.hpp"

#include <imgui.h>

#include <cstdlib>

#include <sstream>

#include "../Logging.hpp"

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
#include "../ThemeManager.hpp"

#include "../texture/RvlImageConvert.hpp"
#include "../texture/CtrImageConvert.hpp"

#include "../command/CommandModifySpritesheet.hpp"
#include "../command/CommandModifyArrangements.hpp"

#include "../App/Popups.hpp"

#include "../Easings.hpp"

#include "../stb/stb_rect_pack.h"

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

#ifdef __WIN32__
    commandStream <<
        "cmd.exe /c \"\"" <<
        config.imageEditorPath <<
        "\" \"" << imagePath << "\"\"";
#else
    commandStream <<
        "\"" <<
        config.imageEditorPath <<
        "\" \"" << imagePath << "\"";
#endif // __WIN32__

    std::string command = commandStream.str();
    Logging::info << "[WindowSpritesheet::RunEditor] Running command: " << command << std::endl;

    std::thread([command]() {
        std::system(command.c_str());
    }).detach();
}

void WindowSpritesheet::FormatPopup() {
    BEGIN_GLOBAL_POPUP();

    CENTER_NEXT_WINDOW();

    ImGui::SetNextWindowSize({ 800.f, 550.f }, ImGuiCond_Appearing);

    static bool lateOpen { false };
    bool open = ImGui::BeginPopupModal("Re-encode image..###ReencodePopup", nullptr);

    if (open) {
        SessionManager& sessionManager = SessionManager::getInstance();

        std::shared_ptr cellanimSheet =
            sessionManager.getCurrentSession()->getCurrentCellanimSheet();

        const bool isRVL = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL;

        constexpr std::array<TPL::TPLImageFormat, 3> rvlFormats = {
            TPL::TPL_IMAGE_FORMAT_RGBA32,
            TPL::TPL_IMAGE_FORMAT_RGB5A3,
            TPL::TPL_IMAGE_FORMAT_CMPR
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
            for (unsigned i = 0; i < rvlFormats.size(); i++)
                names[i] = TPL::getImageFormatName(rvlFormats[i]);
            return names;
        }();
        constexpr std::array ctrFormatNames = [&ctrFormats] {
            std::array<const char*, ctrFormats.size()> names {};
            for (unsigned i = 0; i < ctrFormats.size(); i++)
                names[i] = CTPK::getImageFormatName(ctrFormats[i]);
            return names;
        }();
        
        static int selectedFormatIndex { 0 };
        static int mipCount { 1 };

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

                RvlImageConvert::fromRGBA32(
                    imageBuffer, nullptr, nullptr, rvlFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                    imageData
                );
    
                // Swap buffers.
                RvlImageConvert::toRGBA32(
                    imageData, rvlFormats[selectedFormatIndex],
                    cellanimSheet->getWidth(), cellanimSheet->getHeight(),
                    imageBuffer, nullptr
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

            this->formattingNewTex->LoadRGBA32(
                imageData,
                cellanimSheet->getWidth(), cellanimSheet->getHeight()
            );
            
            if (isRVL)
                this->formattingNewTex->setTPLOutputFormat(rvlFormats[selectedFormatIndex]);
            else
                this->formattingNewTex->setCTPKOutputFormat(ctrFormats[selectedFormatIndex]);

            delete[] imageData;
        };

        if (!lateOpen) {
            mipCount = cellanimSheet->getOutputMipCount();

            this->formattingNewTex = std::make_shared<Texture>();

            unsigned char* imageData = cellanimSheet->GetRGBA32();
            if (imageData) {
                this->formattingNewTex->LoadRGBA32(
                    imageData,
                    cellanimSheet->getWidth(), cellanimSheet->getHeight()
                );
                delete[] imageData;

                this->formattingNewTex->setOutputMipCount(mipCount);

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
                ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
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
                        );
                    }
                    else {
                        dataSize = CtrImageConvert::getImageByteSize(
                            ctrFormats[selectedFormatIndex], imageWidth, imageHeight, mipCount
                        );
                    }
                        

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
                ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
                ImGui::SeparatorText("Format Info");
                ImGui::PopFont();

                if (ImGui::BeginChild(
                    "FormatInfoChild",
                    { 0.f, 0.f },
                    ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY
                )) {
                    const char* colorDesc = "";
                    const char* alphaDesc = "";

                    if (isRVL) {
                        switch (rvlFormats[selectedFormatIndex]) {
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
                        case TPL::TPL_IMAGE_FORMAT_CMPR:
                            colorDesc = "Colors: 16-bit (interpolated), 65536 colors";
                            alphaDesc = "Alpha: 1-bit, either opaque or transparent";
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

                    ImGui::BulletText("%s", colorDesc);
                    ImGui::BulletText("%s", alphaDesc);
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

                this->formattingNewTex->setOutputMipCount(mipCount);
            }

            ImGui::EndChild();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();

            if (ImGui::Button("Apply")) {
                this->formattingNewTex->setName(cellanimSheet->getName());

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifySpritesheet>(
                    sessionManager.getCurrentSession()
                        ->getCurrentCellanim().object->sheetIndex,
                    this->formattingNewTex
                ));

                this->formattingNewTex.reset();

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

            if (this->formattingNewTex)
                ImGui::GetWindowDrawList()->AddImage(
                    this->formattingNewTex->getImTextureId(),
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

// TODO: move this somewhere else
bool RepackSheet() {
    constexpr int PADDING = 4;
    constexpr int PADDING_HALF = PADDING / 2;
    constexpr uint32_t BORDER_COLOR = IM_COL32_BLACK;

    SessionManager& sessionManager = SessionManager::getInstance();

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
                .w = static_cast<int>(part.regionW) + PADDING,
                .h = static_cast<int>(part.regionH) + PADDING,
                .x = static_cast<int>(part.regionX) - PADDING_HALF,
                .y = static_cast<int>(part.regionY) - PADDING_HALF,
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
    std::unique_ptr<unsigned char[]> newImage(new unsigned char[pixelCount * 4]);
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
                .w = static_cast<int>(part.regionW) + PADDING,
                .h = static_cast<int>(part.regionH) + PADDING,
                .x = static_cast<int>(part.regionX) - PADDING_HALF,
                .y = static_cast<int>(part.regionY) - PADDING_HALF,
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

    newTexture->setName(cellanimSheet->getName());

    session.addCommand(std::make_shared<CommandModifySpritesheet>(
        cellanimObject->sheetIndex, newTexture
    ));
    session.addCommand(std::make_shared<CommandModifyArrangements>(
        session.getCurrentCellanimIndex(), std::move(arrangements)
    ));

    return true;
}

// TODO: move this somewhere else
bool FixSheetEdgePixels() {
    SessionManager& sessionManager = SessionManager::getInstance();

    auto& session = *sessionManager.getCurrentSession();

    std::shared_ptr cellanimObject = session.getCurrentCellanim().object;
    std::shared_ptr cellanimSheet = session.getCurrentCellanimSheet();

    const unsigned width = cellanimSheet->getWidth();
    const unsigned height = cellanimSheet->getHeight();
    const unsigned pixelCount = cellanimSheet->getPixelCount();

    std::unique_ptr<unsigned char[]> texture(new unsigned char[pixelCount * 4]());
    cellanimSheet->GetRGBA32(texture.get());

    // Attempt to 'fix' every pixel
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            unsigned pixelIndex = (y * width + x) * 4;

            // Pixel is #00000000
            if (*(uint32_t*)(texture.get() + pixelIndex) == 0) {
                static const int offsets[4][2] = {
                    // left, right,  up,      down
                    {-1, 0}, {1, 0}, {0, -1}, {0, 1}
                };

                for (unsigned i = 0; i < 4; i++) {
                    int nx = x + offsets[i][0];
                    int ny = y + offsets[i][1];

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        unsigned neighborIndex = (ny * width + nx) * 4;

                        if (texture[neighborIndex + 3] > 0) {
                            texture[pixelIndex + 0] = texture[neighborIndex + 0];
                            texture[pixelIndex + 1] = texture[neighborIndex + 1];
                            texture[pixelIndex + 2] = texture[neighborIndex + 2];
                            break;
                        }
                    }
                }
            }
        }
    }

    cellanimSheet->LoadRGBA32(texture.get(), width, height);

    return true;
}

void WindowSpritesheet::Update() {
    static bool firstOpen { true };
    if (firstOpen) {
        this->gridType = ThemeManager::getInstance().getThemeIsLight() ?
            GridType_Light : GridType_Dark;

        firstOpen = false;
    }

    SessionManager& sessionManager = SessionManager::getInstance();

    std::shared_ptr cellanimSheet = sessionManager.getCurrentSession()->getCurrentCellanimSheet();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::Begin("Spritesheet", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImGuiIO& io = ImGui::GetIO();

    const ImVec2 windowSize = ImGui::GetContentRegionAvail();

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
                        gridType = ThemeManager::getInstance().getThemeIsLight() ? GridType_Light : GridType_Dark;
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
                const ConfigManager& configManager = ConfigManager::getInstance();

                if (cellanimSheet->ExportToFile(configManager.getConfig().textureEditPath.c_str())) {
                    OPEN_GLOBAL_POPUP("###WaitForModifiedTexture");
                    this->RunEditor();
                }
                else {
                    OPEN_GLOBAL_POPUP("###TextureExportFailed");
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

                    std::shared_ptr<Texture> newTexture = std::make_shared<Texture>();
                    bool ok = newTexture->LoadSTBFile(openFileDialog);

                    if (ok) {
                        bool diffSize =
                            newTexture->getWidth()  != cellanimSheet->getWidth() ||
                            newTexture->getHeight() != cellanimSheet->getHeight();

                        Popups::_oldTextureSizeX = cellanimSheet->getWidth();
                        Popups::_oldTextureSizeY = cellanimSheet->getHeight();

                        newTexture->setName(cellanimSheet->getName());

                        sessionManager.getCurrentSession()->addCommand(
                        std::make_shared<CommandModifySpritesheet>(
                            sessionManager.getCurrentSession()
                                ->getCurrentCellanim().object->sheetIndex,
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
                bool ok = RepackSheet();
                if (!ok)
                    OPEN_GLOBAL_POPUP("###SheetRepackFailed");
            }

            if (ImGui::MenuItem((const char*)ICON_FA_STAR " Fix sheet transparency", nullptr, false)) {
                FixSheetEdgePixels();
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

    uint32_t backgroundColor { IM_COL32(0,0,0,0xFF) };
    switch (this->gridType) {
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
            customGridColor.x * 255,
            customGridColor.y * 255,
            customGridColor.z * 255,
            customGridColor.w * 255
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

    float scale = fitRect(imageRect, canvasSize);

    if (draggingCanvas) {
        this->sheetZoomedOffset.x += io.MouseDelta.x;
        this->sheetZoomedOffset.y += io.MouseDelta.y;

        this->sheetZoomedOffset.x = std::clamp<float>(this->sheetZoomedOffset.x, -(imageRect.x / 2), imageRect.x / 2);
        this->sheetZoomedOffset.y = std::clamp<float>(this->sheetZoomedOffset.y, -(imageRect.y / 2), imageRect.y / 2);
    }

    ImVec2 canvasOffset;

    if (this->sheetZoomTriggered) {
        float rel = (ImGui::GetTime() - this->sheetZoomTimer) / SHEET_ZOOM_TIME;
        float rScale = (this->sheetZoomEnabled ? Easings::In : Easings::Out)(
            this->sheetZoomEnabled ? 1.f - rel : rel
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
        cellanimSheet->getImTextureId(),
        imagePosition,
        { imagePosition.x + imageRect.x, imagePosition.y + imageRect.y, }
    );

    AppState& appState = AppState::getInstance();

    if (this->drawBounding) {
        PlayerManager& playerManager = PlayerManager::getInstance();

        const auto& session = sessionManager.getCurrentSession();

        const auto& textureGroup = session->sheets;
        const std::shared_ptr cellanimObject = session->getCurrentCellanim().object;

        const auto& arrangement = playerManager.getArrangement();

        for (unsigned i = 0; i < arrangement.parts.size(); i++) {
            if (appState.focusOnSelectedPart && !appState.isPartSelected(i))
                continue;

            const auto& part = arrangement.parts[i];

            unsigned sourceRect[4] {
                part.regionX % cellanimObject->sheetW,
                part.regionY % cellanimObject->sheetH,
                part.regionW, part.regionH
            };

            const auto& associatedTex = textureGroup->getTextureByVarying(part.textureVarying);

            float mismatchScaleX =
                static_cast<float>(associatedTex->getWidth()) / cellanimObject->sheetW;
            float mismatchScaleY =
                static_cast<float>(associatedTex->getHeight()) / cellanimObject->sheetH;

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
