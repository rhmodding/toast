#ifndef POPUP_EDITANIMATIONNAME_HPP
#define POPUP_EDITANIMATIONNAME_HPP

#include <imgui.h>

#include <string>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyAnimationName.hpp"

#include "../../common.hpp"

static void Popup_EditAnimationName(int animationIndex) {
    if (animationIndex < 0)
        return;

    static bool lateOpen { false };

    static char newMacro[256] { '\0' };

    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    bool open = ImGui::BeginPopupModal("Edit animation macro###EditAnimationName", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    SessionManager& sessionManager = SessionManager::getInstance();

    if (!lateOpen && open) {
        const auto& animation =
            sessionManager.getCurrentSession()
                ->getCurrentCellanim().object
                ->animations.at(animationIndex);

        if (animation.name.empty()) {
            const std::string& str = sessionManager.getCurrentSession()->getCurrentCellanim().name;
            snprintf(newMacro, sizeof(newMacro), "%.*s_", static_cast<int>(str.size() - 6), str.c_str());
        }
        else {
            strncpy(newMacro, animation.name.c_str(), sizeof(newMacro) - 1);
        }
    }

    if (open) {
        ImGui::Text("Edit macro name for animation no. %u:", animationIndex + 1);
        ImGui::InputText("##Input", newMacro, sizeof(newMacro));

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("OK", { 120.f, 0.f })) {
            // Replace spaces with underscores
            {
                char* character = newMacro;

                while (*character != '\0') {
                    if (*character == ' ')
                        *character = '_';
                    character++;
                }
            }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyAnimationName>(
                sessionManager.getCurrentSession()->currentCellanim,
                animationIndex,
                newMacro
            ));

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    lateOpen = open;
}

#endif // POPUP_EDITANIMATIONNAME_HPP
