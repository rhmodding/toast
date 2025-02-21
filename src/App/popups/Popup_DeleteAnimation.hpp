#ifndef POPUP_DELETEANIMATION_HPP
#define POPUP_DELETEANIMATION_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../command/CommandDeleteAnimation.hpp"

#include "../../common.hpp"

static void Popup_DeleteAnimation(int animationIndex) {
    if (animationIndex < 0)
        return;

    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Are you sure?###DeleteAnimation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(
            "Are you sure you want to delete animation no. %d?\n"
            "This change will shift other animations indices.",
            animationIndex
        );

        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Delete animation")) {
            ImGui::CloseCurrentPopup();

            SessionManager& sessionManager = SessionManager::getInstance();

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandDeleteAnimation>(
                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                animationIndex
            ));
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_TEXTUREEXPORTFAILED_HPP
