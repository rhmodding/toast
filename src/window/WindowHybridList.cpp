#include "WindowHybridList.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <sstream>
#include <string>

#include <cmath>

#include "../AppState.hpp"
#include "../SessionManager.hpp"

void WindowHybridList::FlashWindow() {
    this->flashWindow = true;
    this->flashTimer = static_cast<float>(ImGui::GetTime());
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
    adjustedColor.w = color.w;
    
    adjustedColor.x = std::min(std::max(adjustedColor.x, 0.0f), 1.f);
    adjustedColor.y = std::min(std::max(adjustedColor.y, 0.0f), 1.f);
    adjustedColor.z = std::min(std::max(adjustedColor.z, 0.0f), 1.f);

    return adjustedColor;
}

#define WINDOW_FLASH_TIME .3f // seconds

void WindowHybridList::Update() {
    GET_APP_STATE;

    static bool lastArrangementMode{ appState.getArrangementMode() };
    if (lastArrangementMode != appState.getArrangementMode()) {
        this->flashTrigger = true;
        this->FlashWindow();
    }

    lastArrangementMode = appState.getArrangementMode();

    if (this->flashWindow && (static_cast<float>(ImGui::GetTime()) - this->flashTimer > WINDOW_FLASH_TIME))
        ResetFlash();

    if (this->flashWindow) {
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            adjustBrightness(
                color,
                (this->flashTimer - static_cast<float>(ImGui::GetTime())) + WINDOW_FLASH_TIME
            )
        );
    }

    ImGui::Begin(appState.getArrangementMode() ? "Arrangements###HybridList" : "Animations###HybridList", nullptr);

    ImGui::BeginChild("List", ImVec2(0, 0), ImGuiChildFlags_Border);
    {
        GET_ANIMATABLE;

        if (!appState.getArrangementMode())
            for (uint16_t n = 0; n < globalAnimatable->cellanim->animations.size(); n++) {
                std::stringstream fmtStream;

                GET_SESSION_MANAGER;

                auto query = sessionManager.getCurrentSession()->getAnimationNames().find(n);

                const char* animName =
                    query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
                        query->second.c_str() :
                        "(no macro defined)";

                fmtStream << std::to_string(n+1) << ". " << animName;

                if (ImGui::Selectable(fmtStream.str().c_str(), appState.selectedAnimation == n, ImGuiSelectableFlags_SelectOnNav)) {
                    appState.selectedAnimation = n;

                    globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);

                    appState.playerState.currentFrame = 0;
                    appState.playerState.updateSetFrameCount();
                    appState.playerState.updateCurrentFrame();

                    RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);
                }
            }
        else
            for (uint16_t n = 0; n < globalAnimatable->cellanim->arrangements.size(); n++) {
                char buffer[32];
                sprintf(buffer, "Arrangement no. %d", n+1);

                RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

                if (ImGui::Selectable(buffer, appState.controlKey.arrangementIndex == n, ImGuiSelectableFlags_SelectOnNav)) {
                    appState.controlKey.arrangementIndex = n;

                    // Update arrangementPtr
                    arrangementPtr = globalAnimatable->getCurrentArrangement();

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);
                }
            }
    }
    ImGui::EndChild();

    ImGui::End();

    if (this->flashWindow)
        ImGui::PopStyleColor();
}