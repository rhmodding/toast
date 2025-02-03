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

    this->DrawPreview(&globalAnimatable);

    ImGui::SameLine();

    unsigned animationIndex = globalAnimatable.getCurrentAnimationIndex();

    const char* animName = globalAnimatable.cellanim->animations.at(animationIndex).name.c_str();
    if (animName == nullptr)
        animName = "(no macro defined)";

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Animation no. %u:", animationIndex + 1);

        ImGui::PushFont(appState.fonts.large);
        ImGui::TextWrapped("%s", animName);
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();


    ImGui::SeparatorText((const char*)ICON_FA_PENCIL " Properties");
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
        ImGui::Bullet(); ImGui::SameLine();
        ImGui::TextWrapped(
            "Macro names are not used for mapping animations directly in the game.\n"
            "To swap animations, use [Swap index..]."
        );
    ImGui::Unindent();
}
