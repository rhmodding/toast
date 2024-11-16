#ifndef POPUP_DELETEANIMATION
#define POPUP_DELETEANIMATION

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../command/CommandDeleteAnimation.hpp"

#include "../../common.hpp"

void Popup_DeleteAnimation(int animationIndex) {
    if (animationIndex < 0)
        return;

    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Are you sure?###DeleteAnimation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(
            "Are you sure you want to delete animation no. %i?\n"
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

            GET_SESSION_MANAGER;

            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandDeleteAnimation>(
                sessionManager.getCurrentSession()->currentCellanim,
                animationIndex
            ));
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_TEXTUREEXPORTFAILED
