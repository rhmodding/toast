#include "WindowConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>

#include "manager/ThemeManager.hpp"

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

static bool textInputStdString(const char* label, std::string& str) {
    constexpr ImGuiTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    return ImGui::InputText(label, const_cast<char*>(str.c_str()), str.capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) -> int {
        std::string* str = reinterpret_cast<std::string*>(data->UserData);
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            // Resize string callback
            //
            // If for some reason we refuse the new length (BufTextLen) and/or
            // capacity (BufSize) we need to set them back to what we want.
            IM_ASSERT(data->Buf == str->c_str());

            str->resize(data->BufTextLen);
            data->Buf = const_cast<char*>(str->c_str());
        }

        return 0;
    }, &str);
}

void WindowConfig::Update() {
    ConfigManager& configManager = ConfigManager::getInstance();

    if (!mOpen)
        return;

    if (mFirstOpen) {
        mMyConfig = configManager.getConfig();
        mFirstOpen = false;
    }

    CENTER_NEXT_WINDOW();

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

            ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
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
                enum CompressionLevels { CL_Highest, CL_High, CL_Medium, CL_Low, CL_VeryLow, CL_Count };

                static const char* compressionLevelNames[CL_Count] = { "Highest", "High", "Medium", "Low", "Very Low" };
                static const int compressionLevelMap[CL_Count] = { 9, 8, 7, 4, 2 };
                static const int compressionLevels[] = {
                    CL_Count, // 0
                    CL_Count, // 1
                    CL_VeryLow, // 2
                    CL_Count, // 3
                    CL_Low, // 4
                    CL_Count, // 5
                    CL_Count, // 6
                    CL_Medium, // 7
                    CL_High, // 8
                    CL_Highest // 9
                };

                int selectedCompressionLevel = compressionLevels[mMyConfig.compressionLevel];

                char buffer[64];

                if (selectedCompressionLevel >= 0 && selectedCompressionLevel < CL_Count)
                    strncpy(buffer, compressionLevelNames[selectedCompressionLevel], sizeof(buffer));
                else
                    snprintf(buffer, sizeof(buffer), "Other (%d)", mMyConfig.compressionLevel);

                if (ImGui::SliderInt(
                    "Compression level",
                    &selectedCompressionLevel,
                    CL_Count - 1, 0,
                    buffer,
                    ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp
                )) {
                    mMyConfig.compressionLevel = compressionLevelMap[selectedCompressionLevel];
                }

                static const char* etc1QualityNames[(int)ETC1Quality::Count] = { "Low", "Medium", "High" };
                if (mMyConfig.etc1Quality < ETC1Quality::Count)
                    strncpy(buffer, etc1QualityNames[(int)mMyConfig.etc1Quality], sizeof(buffer));
                else
                    strncpy(buffer, "Invalid", sizeof(buffer));

                ImGui::SliderInt(
                    "ETC1 compression quality",
                    reinterpret_cast<int*>(&mMyConfig.etc1Quality),
                    0, (int)ETC1Quality::Count - 1,
                    buffer,
                    ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp
                );
            } break;

            case Category_Theming: {
                static const char* themeOptions[] { "Light", "Dark" };
                ImGui::Combo("Theme", reinterpret_cast<int*>(&mMyConfig.theme), themeOptions, 2);
            } break;

            case Category_Paths: {
                textInputStdString("Image editor path", mMyConfig.imageEditorPath);
                textInputStdString("Texture edit path", mMyConfig.textureEditPath);
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
                configManager.SaveConfig();

                ThemeManager::getInstance().applyTheming();
            }
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}
