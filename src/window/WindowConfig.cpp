#include "WindowConfig.hpp"

#include "../AppState.hpp"

#include <imgui.h>

#include <cstdint>

#include <stdio.h>

enum Categories: uint8_t {
    Category_General,
    Category_Theming,
    Category_Paths,

    Categories_End
};

const char* categoryNames[] = {
    "General",
    "Theming",
    "Paths"
};

void WindowConfig::Update() {
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Config", &this->open, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Config")) {
                if (ImGui::MenuItem("Clear..", nullptr)) {
                    this->selfConfig = ConfigManager::Config{};
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Left
        static uint8_t selected{ 0 };
        {
            ImGui::BeginChild("Categories", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
            for (uint8_t i = 0; i < Categories_End; i++) {
                if (ImGui::Selectable(categoryNames[i], selected == i))
                    selected = i;
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("Properties", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, -2 });

            ImGui::TextWrapped("Category");

            ImGui::PushFont(AppState::getInstance().fontLarge);
            ImGui::TextWrapped("%s", categoryNames[selected]);
            ImGui::PopFont();

            ImGui::PopStyleVar();

            ImGui::Dummy({ 0, 5 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            switch (selected) {
                // TODO: implement other categories

                case Category_General: {
                    ImGui::Checkbox("Show unknown fields/values", &this->selfConfig.showUnknownValues);
                } break;

                case Category_Theming: {
                    int currentTheme = this->selfConfig.darkTheme ? 0 : 1;

                    const char* themeOptions[] = { "Dark", "Light" };
                    if (ImGui::Combo("Theme", &currentTheme, themeOptions, 2))
                        this->selfConfig.darkTheme = currentTheme == 0;
                } break;
                
                default:
                    ImGui::TextWrapped("No options avaliable.");
                    break;
            }

            ImGui::EndChild();

            GET_CONFIG_MANAGER;

            ImGui::BeginDisabled(this->selfConfig == configManager.config);
            if (ImGui::Button("Revert")) {
                this->selfConfig = configManager.config;
            }
            ImGui::SameLine();

            if (ImGui::Button("Save & Apply")) {
                configManager.config = this->selfConfig;
                configManager.SaveConfig();

                AppState::getInstance().UpdateTheme();
            }
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}