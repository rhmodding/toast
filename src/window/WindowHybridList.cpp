#include "WindowHybridList.hpp"

#include "imgui.h"

void WindowHybridList::Update() {
    GET_APP_STATE;

    ImGui::Begin("Animations###HybridList", nullptr);

    ImGui::BeginChild("AnimationList", ImVec2(0, 0), ImGuiChildFlags_Border);
    {
        for (uint16_t n = 0; n < this->animatable->cellanim->animations.size(); n++) {
            char buffer[128];

            auto query = this->animationNames->find(n);
            sprintf_s(
                buffer, 128, "%d. %s", n,

                query != this->animationNames->end() ?
                    query->second.c_str() :
                    "(no macro defined)"
            );

            if (ImGui::Selectable(buffer, this->selectedAnimation == n)) {
                this->selectedAnimation = n;

                this->animatable->setAnimation(this->selectedAnimation);

                appState.playerState.currentFrame = 0;
                appState.playerState.updateSetFrameCount();
                appState.playerState.updateCurrentFrame();
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}