#include "../WindowInspector.hpp"

#include <sstream>

#include "../../SessionManager.hpp"

#include "../../anim/Animatable.hpp"

#include "../../command/CommandModifyAnimationName.hpp"
#include "../../command/CommandSwapAnimations.hpp"

#include "../../font/FontAwesome.h"

#include "../../AppState.hpp"

#include "../../App/Popups.hpp"

void WindowInspector::Level_Animation() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    this->DrawPreview(&globalAnimatable);

    ImGui::SameLine();

    const uint16_t animationIndex = globalAnimatable.getCurrentAnimationIndex();
    auto query = sessionManager.getCurrentSession()->getAnimationNames().find(animationIndex);

    const char* animationName =
        query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
            query->second.c_str() : nullptr;

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Animation no. %u:", animationIndex + 1);

        ImGui::PushFont(appState.fonts.large);
        ImGui::TextWrapped("%s", animationName ? animationName : "(no macro defined)");
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();


    RvlCellAnim::Animation* animation = globalAnimatable.getCurrentAnimation();

    ImGui::SeparatorText((char*)ICON_FA_PENCIL " Properties");
    if (ImGui::Button("Edit macro name..")) {
        Popups::_editAnimationNameIdx = animationIndex;
        appState.OpenGlobalPopup("###EditAnimationName");
    }

    ImGui::SameLine();
    if (ImGui::Button("Swap index..")) {
        Popups::_swapAnimationIdx = animationIndex;
        appState.OpenGlobalPopup("###SwapAnimation");
    }

    ImGui::Indent();
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextWrapped("Macro names are not used for mapping animations directly in the game.\nTo swap animations, use [Swap index..].");
    ImGui::Unindent();
}
