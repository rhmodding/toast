#include "../WindowInspector.hpp"

#include <sstream>

#include "../../SessionManager.hpp"

#include "../../ThemeManager.hpp"

#include "../../AppState.hpp"

#include "../../anim/CellanimRenderer.hpp"

#include "../../command/CommandSetAnimationInterpolated.hpp"

#include "../../font/FontAwesome.h"

#include "../../App/Popups.hpp"

void WindowInspector::Level_Animation() {
    PlayerManager& playerManager = PlayerManager::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();

    this->drawPreview();

    const bool isCtr = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_CTR;

    ImGui::SameLine();

    unsigned animationIndex = playerManager.getAnimationIndex();
    const char* animationName = playerManager.getAnimation().name.c_str();
    if (animationName[0] == '\0')
        animationName = "(no name set)";

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Animation no. %u:", animationIndex + 1);

        ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
        ImGui::TextWrapped("%s", animationName);
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::SeparatorText((const char*)ICON_FA_PENCIL " Properties");

    if (isCtr) {
        bool isInterpolated = playerManager.getAnimation().isInterpolated;

        if (ImGui::Checkbox("Interpolated", &isInterpolated)) {
            const auto& currentSession = sessionManager.getCurrentSession();

            currentSession->addCommand(
            std::make_shared<CommandSetAnimationInterpolated>(
                currentSession->getCurrentCellanimIndex(),
                animationIndex,
                isInterpolated
            ));
        }

        ImGui::Dummy({ 0.f, 5.f });
    }

    if (ImGui::Button("Edit animation name..")) {
        Popups::_editAnimationNameIdx = animationIndex;
        OPEN_GLOBAL_POPUP("###EditAnimationName");
    }

    ImGui::SameLine();
    if (ImGui::Button("Swap animation..")) {
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
