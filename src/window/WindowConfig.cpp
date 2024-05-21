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

bool CPPSTRING_InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr) {
    struct InputTextCallback_UserData {
        std::string*            Str;
        ImGuiInputTextCallback  ChainCallback;
        void*                   ChainCallbackUserData;
    };
    
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) -> int {
        InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            // Resize string callback
            // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
            std::string* str = user_data->Str;
            IM_ASSERT(data->Buf == str->c_str());
            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }
        else if (user_data->ChainCallback)
        {
            // Forward to user callback, if any
            data->UserData = user_data->ChainCallbackUserData;
            return user_data->ChainCallback(data);
        }
        return 0;
    }, &cb_user_data);
}

void WindowConfig::Update() {
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );

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
                case Category_General: {
                    ImGui::Checkbox("Show unknown fields/values", &this->selfConfig.showUnknownValues);

                    ImGui::Separator();

                    ImGui::Checkbox("Enable panning canvas with LMB", &this->selfConfig.canvasLMBPanEnabled);

                    ImGui::Separator();

                    uint32_t updateRate = this->selfConfig.updateRate;
                    if (ImGui::DragScalar("Update rate", ImGuiDataType_U32, &updateRate, .5f, nullptr, nullptr, "%u ups"))
                        this->selfConfig.updateRate = std::clamp<uint32_t>(updateRate, 1, UINT32_MAX);
                } break;

                case Category_Theming: {
                    int currentTheme = this->selfConfig.darkTheme ? 0 : 1;

                    const char* themeOptions[] = { "Dark", "Light" };
                    if (ImGui::Combo("Theme", &currentTheme, themeOptions, 2))
                        this->selfConfig.darkTheme = currentTheme == 0;
                } break;

                case Category_Paths: {
                    CPPSTRING_InputText("Image editor path", &this->selfConfig.imageEditorPath);
                    CPPSTRING_InputText("Texture edit path", &this->selfConfig.textureEditPath);
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
                AppState::getInstance().UpdateUpdateRate();
            }
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}