#include "WindowConfig.hpp"

#include <cstdint>

#include <imgui.h>
#include <imgui_internal.h>

#include <tinyfiledialogs.h>

#include "manager/ThemeManager.hpp"

#include "util/UIUtil.hpp"

#include "font/FontAwesome.h"

enum Categories {
    Category_General,
    Category_Export,
    Category_Theming,
    Category_Paths,

    Categories_Count
};

static const char* categoryNames[Categories_Count] {
    "General",
    "Export",
    "Theming",
    "Paths"
};

void WindowConfig::update() {
    ConfigManager& configManager = ConfigManager::getInstance();

    if (!mOpen)
        return;

    if (mFirstOpen) {
        mMyConfig = configManager.getConfig();
        mFirstOpen = false;
    }

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    ImGui::SetNextWindowSize({ 500.f, 440.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Config", &mOpen, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Config")) {
                if (ImGui::MenuItem("Reset to defaults", nullptr)) {
                    mMyConfig = Config {};
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Left
        static unsigned selected { 0 };
        {
            ImGui::BeginChild("Categories", { 150.f, 0.f }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
            for (unsigned i = 0; i < Categories_Count; i++) {
                if (ImGui::Selectable(categoryNames[i], selected == i))
                    selected = i;
            }
            ImGui::EndChild();
        }

        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("Properties", { 0.f, -ImGui::GetFrameHeightWithSpacing() });

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, -2.f });

            ImGui::TextWrapped("Category");

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.4f);
            ImGui::TextWrapped("%s", categoryNames[selected]);
            ImGui::PopFont();

            ImGui::PopStyleVar();

            ImGui::Dummy({ 0.f, 5.f });
            ImGui::Separator();
            ImGui::Dummy({ 0.f, 5.f });

            switch (selected) {
            case Category_General: {
                ImGui::Checkbox("Enable panning canvas with LMB", &mMyConfig.canvasLMBPanEnabled);

                ImGui::Checkbox("Allow creation of new animations", &mMyConfig.allowNewAnimCreate);

                ImGui::Separator();

                static const unsigned min = 30;

                unsigned updateRate = mMyConfig.updateRate;
                if (ImGui::DragScalar("App update rate", ImGuiDataType_U32, &updateRate, .5f, &min, nullptr, "%u cycles per second")) {
                    mMyConfig.updateRate = std::max<unsigned>(updateRate, min);
                }

                ImGui::Separator();

                static const char* backupOptions[] {
                    "Don't backup",
                    "Backup (don't overwrite last backup)",
                    "Backup (always overwrite last backup)"
                };
                ImGui::Combo("Backup behaviour", reinterpret_cast<int*>(&mMyConfig.backupBehaviour), backupOptions, 3);
            } break;

            case Category_Export: {
                enum class AbstrCompressionLevel {
                    VeryLow,
                    Low,
                    Medium,
                    High,
                    Highest,
                    Count
                };

                static constexpr std::array<std::string_view, static_cast<int>(AbstrCompressionLevel::Count)> abstrCompressionLevelNames = {
                    "Very Low", "Low", "Medium", "High", "Highest"
                };
                static constexpr std::array<int, static_cast<int>(AbstrCompressionLevel::Count)> abstrCompressionLevelValues = {
                    2, 4, 7, 8, 9
                };

                // Map from true compression level (index) to abstract compression level (value).
                static constexpr std::array<AbstrCompressionLevel, 10> abstrCompressionLevelTable = {
                    AbstrCompressionLevel::Count,   // 0
                    AbstrCompressionLevel::Count,   // 1
                    AbstrCompressionLevel::VeryLow, // 2
                    AbstrCompressionLevel::Count,   // 3
                    AbstrCompressionLevel::Low,     // 4
                    AbstrCompressionLevel::Count,   // 5
                    AbstrCompressionLevel::Count,   // 6
                    AbstrCompressionLevel::Medium,  // 7
                    AbstrCompressionLevel::High,    // 8
                    AbstrCompressionLevel::Highest  // 9
                };

                int selectedCompLevelIndex = static_cast<int>(abstrCompressionLevelTable[mMyConfig.compressionLevel]);

                char buffer[64];
                if (selectedCompLevelIndex >= 0 && selectedCompLevelIndex < static_cast<int>(AbstrCompressionLevel::Count)) {
                    std::snprintf(buffer, sizeof(buffer), "%s", abstrCompressionLevelNames[selectedCompLevelIndex].data());
                }
                else {
                    std::snprintf(buffer, sizeof(buffer), "Other (%d)", mMyConfig.compressionLevel);
                }

                if (ImGui::SliderInt(
                    "Zlib compression level",
                    &selectedCompLevelIndex,
                    0,
                    static_cast<int>(AbstrCompressionLevel::Count) - 1,
                    buffer,
                    ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp
                )) {
                    mMyConfig.compressionLevel = abstrCompressionLevelValues[selectedCompLevelIndex];
                }

                static constexpr std::array<std::string_view, static_cast<int>(ETC1Quality::Count)> ETC1QualityNames = {
                    "Low", "Medium", "High"
                };

                int qualityIndex = static_cast<int>(mMyConfig.etc1Quality);

                if (qualityIndex >= 0 && qualityIndex < static_cast<int>(ETC1Quality::Count)) {
                    std::snprintf(buffer, sizeof(buffer), "%s", ETC1QualityNames[qualityIndex].data());
                }
                else {
                    std::snprintf(buffer, sizeof(buffer), "Invalid");
                }

                if (ImGui::SliderInt(
                    "ETC1(A4) compression quality",
                    &qualityIndex,
                    0,
                    static_cast<int>(ETC1Quality::Count) - 1,
                    buffer,
                    ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp
                )) {
                    mMyConfig.etc1Quality = static_cast<ETC1Quality>(qualityIndex);
                }
            } break;

            case Category_Theming: {
                static const char* themeNames[static_cast<int>(ThemeChoice::Count)] { "Light", "Dark" };

                int themeIndex = static_cast<int>(mMyConfig.theme);
                if (ImGui::Combo("Theme", &themeIndex, themeNames, 2)) {
                    mMyConfig.theme = static_cast<ThemeChoice>(themeIndex);
                }
            } break;

            case Category_Paths: {
                float buttonSize = ImGui::GetFontSize() + (ImGui::GetStyle().FramePadding.y * 2.0f);
                ImVec2 buttonSizeVec = ImVec2(buttonSize, buttonSize);

                if (ImGui::Button((const char *)ICON_FA_FOLDER_OPEN "##ImageEditorPathLoc", buttonSizeVec)) {
                    const char* filterPatterns[] = {
                #ifdef _WIN32
                    "*.exe"
                #elif __APPLE__
                    // "*.app"
                #endif // __APPLE__, _WIN32
                    };
                    char *openFileDialog = tinyfd_openFileDialog(
                        "Select your preferred image editor",
                        nullptr,
                        ARRAY_LENGTH(filterPatterns), filterPatterns,
                        "Executable program file",
                        false
                    );

                    if (openFileDialog != nullptr) {
                        mMyConfig.imageEditorPath.assign(openFileDialog);
                    }
                }
                ImGui::SameLine(0.0f, 4.0f);
                UIUtil::Widget::StdStringTextInput("Image editor path", mMyConfig.imageEditorPath);

                if (ImGui::Button((const char *)ICON_FA_FOLDER_OPEN "##TextureEditPathLoc", buttonSizeVec)) {
                    const char* filterPatterns[] = { "*.png", "*.tga" };
                    char *saveFileDialog = tinyfd_saveFileDialog(
                        "Select a file to save temporary textures to",
                        nullptr,
                        ARRAY_LENGTH(filterPatterns), filterPatterns,
                        "Image file (.png, .tga)"
                    );

                    if (saveFileDialog != nullptr) {
                        mMyConfig.textureEditPath.assign(saveFileDialog);
                    }
                }
                ImGui::SameLine(0.0f, 4.0f);
                UIUtil::Widget::StdStringTextInput("Temp texture path", mMyConfig.textureEditPath);
            } break;

            default:
                ImGui::TextWrapped("No options avaliable.");
                break;
            }

            ImGui::EndChild();

            ImGui::BeginDisabled(mMyConfig == configManager.getConfig());
            if (ImGui::Button("Revert")) {
                mMyConfig = configManager.getConfig();
            }
            ImGui::SameLine();

            if (ImGui::Button("Save & Apply")) {
                configManager.setConfig(mMyConfig);
                configManager.saveConfig();

                ThemeManager::getInstance().applyTheming();
            }
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}
