#include "WindowConfig.hpp"

#include <imgui.h>

#include <cstdint>

#include "../AppState.hpp"

enum Categories {
    Category_General,
    Category_Export,
    Category_Theming,
    Category_Paths,

    Categories_End
};

const char* categoryNames[] = {
    "General",
    "Export",
    "Theming",
    "Paths"
};

bool CppInputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* userData = nullptr) {
    struct UserData {
        std::string* stringPtr;
        ImGuiInputTextCallback chainCb;
        void* cbUserData;
    } lUserData {
        .stringPtr = str,
        .chainCb = callback,
        .cbUserData = userData
    };

    flags |= ImGuiInputTextFlags_CallbackResize;

    return ImGui::InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) -> int {
        UserData* userData = (UserData*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            // Resize string callback
            //
            // If for some reason we refuse the new length (BufTextLen) and/or
            // capacity (BufSize) we need to set them back to what we want.

            std::string* str = userData->stringPtr;
            IM_ASSERT(data->Buf == str->c_str());

            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }
        else if (userData->chainCb) {
            data->UserData = userData->cbUserData;
            return userData->chainCb(data);
        }

        return 0;
    }, &lUserData);
}

void WindowConfig::Update() {
    static bool firstOpen{ true };

    if (!this->open)
        return;

    if (UNLIKELY(firstOpen)) {
        this->selfConfig = ConfigManager::getInstance().getConfig();

        firstOpen = false;
    }

    CENTER_NEXT_WINDOW;

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
            for (unsigned i = 0; i < Categories_End; i++) {
                if (ImGui::Selectable(categoryNames[i], selected == i))
                    selected = i;
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("Properties", ImVec2(0.f, -ImGui::GetFrameHeightWithSpacing()));

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, -2 });

            ImGui::TextWrapped("Category");

            ImGui::PushFont(AppState::getInstance().fonts.large);
            ImGui::TextWrapped("%s", categoryNames[selected]);
            ImGui::PopFont();

            ImGui::PopStyleVar();

            ImGui::Dummy({ 0, 5 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            switch (selected) {
                case Category_General: {
                    ImGui::Checkbox("Enable panning canvas with LMB", &this->selfConfig.canvasLMBPanEnabled);

                    ImGui::Separator();

                    const unsigned min = 30;

                    unsigned updateRate = this->selfConfig.updateRate;
                    if (ImGui::DragScalar("App update rate", ImGuiDataType_U32, &updateRate, .5f, &min, nullptr, "%u cycles per second"))
                        this->selfConfig.updateRate = std::clamp<unsigned>(updateRate, min, UINT32_MAX);

                    ImGui::Separator();

                    const char* backupOptions[] = { "Don't backup", "Backup (don't overwrite last backup)", "Backup (always overwrite last backup)" };
                    ImGui::Combo("Backup behaviour", reinterpret_cast<int*>(&this->selfConfig.backupBehaviour), backupOptions, 3);
                } break;

                case Category_Export: {
                    enum CompressionLevels { CL_Highest, CL_High, CL_Medium, CL_Low, CL_VeryLow, CL_Count };
                    static const char* levelNames[CL_Count] = { "Highest", "High", "Medium", "Low", "Very Low" };

                    static const int levelMap[CL_Count] = { 9, 8, 7, 4, 2 };

                    int selectedLevel;
                    {
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

                        selectedLevel = compressionLevels[this->selfConfig.compressionLevel];
                    }

                    const char* clName = (selectedLevel >= 0 && selectedLevel < CL_Count) ?
                        levelNames[selectedLevel] : "Other ";

                    if (selectedLevel >= 0 && selectedLevel < CL_Count)
                        clName = levelNames[selectedLevel];
                    else {
                        static char buf[64];
                        snprintf(buf, 64, "Other (lvl %i)", this->selfConfig.compressionLevel);
    
                        clName = buf;
                    }
                    
                    if (ImGui::SliderInt("Compression Level", &selectedLevel, CL_Count - 1, 0, clName, ImGuiSliderFlags_NoInput)) {
                        this->selfConfig.compressionLevel = levelMap[selectedLevel];
                    }
                } break;

                case Category_Theming: {
                    const static char* themeOptions[] = { "Light", "Dark" };
                    ImGui::Combo("Theme", reinterpret_cast<int*>(&this->selfConfig.theme), themeOptions, 2);
                } break;

                case Category_Paths: {
                    CppInputText("Image editor path", &this->selfConfig.imageEditorPath);
                    CppInputText("Texture edit path", &this->selfConfig.textureEditPath);
                } break;

                default:
                    ImGui::TextWrapped("No options avaliable.");
                    break;
            }

            ImGui::EndChild();

            GET_CONFIG_MANAGER;

            ImGui::BeginDisabled(this->selfConfig == configManager.getConfig());
            if (ImGui::Button("Revert")) {
                this->selfConfig = configManager.getConfig();
            }
            ImGui::SameLine();

            if (ImGui::Button("Save & Apply")) {
                configManager.setConfig(this->selfConfig);
                configManager.Save();

                AppState::getInstance().applyTheming();
            }
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}
