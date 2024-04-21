#include "WindowHybridList.hpp"

#include "imgui.h"

#include "GLFW/glfw3.h"

void WindowHybridList::FlashWindow() {
    this->flashWindow = true;
    this->flashTimer = static_cast<float>(glfwGetTime());
}

void WindowHybridList::ResetFlash() {
    this->flashWindow = false;
    this->flashTrigger = false;
}

ImVec4 adjustBrightness(const ImVec4& color, float factor) {
    ImVec4 adjustedColor;

    adjustedColor.x = std::lerp(color.x, 1.f, factor);
    adjustedColor.y = std::lerp(color.y, 1.f, factor);
    adjustedColor.z = std::lerp(color.z, 1.f, factor);
    adjustedColor.w = color.w; // Alpha remains unchanged
    
    // Ensure color components are within [0, 1] range
    adjustedColor.x = std::min(std::max(adjustedColor.x, 0.0f), 1.0f);
    adjustedColor.y = std::min(std::max(adjustedColor.y, 0.0f), 1.0f);
    adjustedColor.z = std::min(std::max(adjustedColor.z, 0.0f), 1.0f);

    return adjustedColor;
}

#define WINDOW_FLASH_TIME .3f // seconds

void WindowHybridList::Update() {
    GET_APP_STATE;

    static bool lastArrangementMode{appState.arrangementMode };
    if (lastArrangementMode != appState.arrangementMode) {
        this->flashTrigger = true;
        this->FlashWindow();
    }

    lastArrangementMode = appState.arrangementMode;

    if (this->flashWindow && (static_cast<float>(glfwGetTime()) - this->flashTimer > WINDOW_FLASH_TIME))
        ResetFlash();

    if (this->flashWindow) {
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            adjustBrightness(
                color,
                (this->flashTimer - static_cast<float>(glfwGetTime())) + WINDOW_FLASH_TIME
            )
        );
    }

    ImGui::Begin(appState.arrangementMode ? "Arrangements###HybridList" : "Animations###HybridList", nullptr);

    ImGui::BeginChild("List", ImVec2(0, 0), ImGuiChildFlags_Border);
    {
        GET_ANIMATABLE;

        if (!appState.arrangementMode)
            for (uint16_t n = 0; n < globalAnimatable->cellanim->animations.size(); n++) {
                char buffer[128];

                auto query = this->animationNames->find(n);
                sprintf_s(
                    buffer, 128, "%d. %s", n,

                    query != this->animationNames->end() ?
                        query->second.c_str() :
                        "(no macro defined)"
                );

                if (ImGui::Selectable(buffer, appState.selectedAnimation == n)) {
                    appState.selectedAnimation = n;

                    globalAnimatable->setAnimation(appState.selectedAnimation);

                    appState.playerState.currentFrame = 0;
                    appState.playerState.updateSetFrameCount();
                    appState.playerState.updateCurrentFrame();

                    RvlCellAnim::Arrangement* arrangementPtr =
                        &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<uint16_t>(arrangementPtr->parts.size() - 1);
                }
            }
        else
            for (uint16_t n = 0; n < globalAnimatable->cellanim->arrangements.size(); n++) {
                char buffer[32];
                sprintf_s(buffer, 32, "Arrangement no. %d", n);

                if (ImGui::Selectable(buffer, appState.controlKey.arrangementIndex == n)) {
                    appState.controlKey.arrangementIndex = n;

                    RvlCellAnim::Arrangement* arrangementPtr =
                        &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<uint16_t>(arrangementPtr->parts.size() - 1);
                }
            }
    }
    ImGui::EndChild();

    ImGui::End();

    if (this->flashWindow)
        ImGui::PopStyleColor();
}