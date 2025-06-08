#ifndef POPUP_EDITANIMATIONNAME_HPP
#define POPUP_EDITANIMATIONNAME_HPP

#include <imgui.h>

#include <string>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyAnimationName.hpp"

#include "../../Macro.hpp"

static void Popup_EditAnimationName(int animationIndex) {
    if (animationIndex < 0)
        return;

    static bool lateOpen { false };

    static char newName[512] { '\0' };

    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    bool open = ImGui::BeginPopupModal("Edit animation name###EditAnimationName", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    SessionManager& sessionManager = SessionManager::getInstance();

    if (!lateOpen && open) {
        const auto& animation =
            sessionManager.getCurrentSession()->getCurrentCellAnim().object
                ->getAnimation(animationIndex);

        strncpy(newName, animation.name.c_str(), sizeof(newName) - 1);
        newName[sizeof(newName) - 1] = '\0';
    }

    if (open) {
        ImGui::Text("Edit name for animation no. %u:", animationIndex + 1);
        ImGui::InputText("##Input", newName, sizeof(newName));

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("OK", { 120.f, 0.f })) {
            // 'Fix' the new name.

            // On RVL we make the name suitable to be a C macro name. It doesn't make sense to
            // do this for CTR cellanims since only RVL stores it's animation names in C header files.
            if (sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL) {
                unsigned i = 0;

                // Prefix a underscore (shifting the string one character to the right)
                if (isdigit(newName[0])) {
                    memmove(newName + 1, newName, strlen(newName) + 1);
                    newName[sizeof(newName) - 1] = '\0';

                    newName[0] = '_';
                    i = 1;
                }

                while (newName[i] != '\0') {
                    if (!isalnum(newName[i]) && newName[i] != '_')
                        newName[i] = '_';
                    i++;
                }
            }
            // On CTR we simply replace spaces with underscores.
            else {
                unsigned i = 0;

                while (newName[i] != '\0') {
                    if (!isalnum(newName[i]) && newName[i] != '_')
                        newName[i] = '_';
                    i++;
                }
            }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyAnimationName>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                animationIndex,
                newName
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
