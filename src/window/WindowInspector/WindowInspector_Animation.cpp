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
    AppState& appState = AppState::getInstance();
    Animatable& globalAnimatable = AppState::getInstance().globalAnimatable;
    SessionManager& sessionManager = SessionManager::getInstance();

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
        OPEN_GLOBAL_POPUP("###EditAnimationName");
    }

    ImGui::SameLine();
    if (ImGui::Button("Swap index..")) {
        Popups::_swapAnimationIdx = animationIndex;
        OPEN_GLOBAL_POPUP("###SwapAnimation");
    }

    ImGui::Indent();
        ImGui::Bullet(); ImGui::SameLine();
        ImGui::TextWrapped(
            "Macro names are not used for mapping animations directly in the game.\n"
            "To swap animations, use [Swap index..]."
        );
    ImGui::Unindent();
}
