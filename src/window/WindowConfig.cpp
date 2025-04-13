#include "WindowConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>

#include "../ThemeManager.hpp"

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

    if (!this->open)
        return;

    if (this->firstOpen) {
        this->selfConfig = configManager.getConfig();
        this->firstOpen = false;
    }

    CENTER_NEXT_WINDOW();

    ImGui::SetNextWindowSize({ 500.f, 440.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Config", &this->open, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Config")) {
                if (ImGui::MenuItem("Reset to defaults", nullptr)) {
                    this->selfConfig = Config {};
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
                ImGui::Checkbox("Enable panning canvas with LMB", &this->selfConfig.canvasLMBPanEnabled);

                ImGui::Separator();

                static const unsigned min = 30;

                unsigned updateRate = this->selfConfig.updateRate;
                if (ImGui::DragScalar("App update rate", ImGuiDataType_U32, &updateRate, .5f, &min, nullptr, "%u cycles per second")) {
                    this->selfConfig.updateRate = std::max<unsigned>(updateRate, min);
                }

                ImGui::Separator();

                static const char* backupOptions[] {
                    "Don't backup",
                    "Backup (don't overwrite last backup)",
                    "Backup (always overwrite last backup)"
                };
                ImGui::Combo("Backup behaviour", reinterpret_cast<int*>(&this->selfConfig.backupBehaviour), backupOptions, 3);
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

                int selectedCompressionLevel = compressionLevels[this->selfConfig.compressionLevel];

                char buffer[64];

                if (selectedCompressionLevel >= 0 && selectedCompressionLevel < CL_Count)
                    strncpy(buffer, compressionLevelNames[selectedCompressionLevel], sizeof(buffer));
                else
                    snprintf(buffer, sizeof(buffer), "Other (%d)", this->selfConfig.compressionLevel);

                if (ImGui::SliderInt(
                    "Compression level",
                    &selectedCompressionLevel,
                    CL_Count - 1, 0,
                    buffer,
                    ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp
                )) {
                    this->selfConfig.compressionLevel = compressionLevelMap[selectedCompressionLevel];
                }

                static const char* etc1QualityNames[ETC1Quality_Count] = { "Low", "Medium", "High" };
                if (this->selfConfig.etc1Quality < ETC1Quality_Count)
                    strncpy(buffer, etc1QualityNames[this->selfConfig.etc1Quality], sizeof(buffer));
                else
                    strncpy(buffer, "Invalid", sizeof(buffer));

                ImGui::SliderInt(
                    "ETC1 compression quality",
                    reinterpret_cast<int*>(&this->selfConfig.etc1Quality),
                    0, ETC1Quality_Count - 1,
                    buffer,
                    ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp
                );
            } break;

            case Category_Theming: {
                static const char* themeOptions[] { "Light", "Dark" };
                ImGui::Combo("Theme", reinterpret_cast<int*>(&this->selfConfig.theme), themeOptions, 2);
            } break;

            case Category_Paths: {
                textInputStdString("Image editor path", this->selfConfig.imageEditorPath);
                textInputStdString("Texture edit path", this->selfConfig.textureEditPath);
            } break;

            default:
                ImGui::TextWrapped("No options avaliable.");
                break;
            }

            ImGui::EndChild();

            ImGui::BeginDisabled(this->selfConfig == configManager.getConfig());
            if (ImGui::Button("Revert")) {
                this->selfConfig = configManager.getConfig();
            }
            ImGui::SameLine();

            if (ImGui::Button("Save & Apply")) {
                configManager.setConfig(this->selfConfig);
                configManager.SaveConfig();

                ThemeManager::getInstance().applyTheming();
            }
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}
