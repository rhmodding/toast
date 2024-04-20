#include "WindowHybridList.hpp"

#include "imgui.h"

void WindowHybridList::Update() {
    GET_APP_STATE;

    ImGui::Begin(appState.arrangementMode ? "Arrangements###HybridList" : "Animations###HybridList", nullptr);

    ImGui::BeginChild("List", ImVec2(0, 0), ImGuiChildFlags_Border);
    {
        if (!appState.arrangementMode)
            for (uint16_t n = 0; n < this->animatable->cellanim->animations.size(); n++) {
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

                    this->animatable->setAnimation(appState.selectedAnimation);

                    appState.playerState.currentFrame = 0;
                    appState.playerState.updateSetFrameCount();
                    appState.playerState.updateCurrentFrame();

                    RvlCellAnim::Arrangement* arrangementPtr =
                        &this->animatable->cellanim->arrangements.at(this->animatable->getCurrentKey()->arrangementIndex);

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<uint16_t>(arrangementPtr->parts.size() - 1);
                }
            }
        else
            for (uint16_t n = 0; n < this->animatable->cellanim->arrangements.size(); n++) {
                char buffer[32];
                sprintf_s(buffer, 32, "Arrangement no. %d", n);

                if (ImGui::Selectable(buffer, appState.controlKey.arrangementIndex == n)) {
                    appState.controlKey.arrangementIndex = n;

                    RvlCellAnim::Arrangement* arrangementPtr =
                        &this->animatable->cellanim->arrangements.at(this->animatable->getCurrentKey()->arrangementIndex);

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<uint16_t>(arrangementPtr->parts.size() - 1);
                }
            }
    }
    ImGui::EndChild();

    ImGui::End();
}