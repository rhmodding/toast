#ifndef POPUP_EDITANIMATIONNAME
#define POPUP_EDITANIMATIONNAME

#include <imgui.h>

#include <string>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyAnimationName.hpp"

#include "../../common.hpp"

#define MACRO_BUF_SIZE 256

void Popup_EditAnimationName(int animationIndex) {
    if (animationIndex < 0)
        return;

    static bool lateOpen{ false };

    static char newMacro[MACRO_BUF_SIZE]{ "" };

    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    bool open = ImGui::BeginPopupModal("Edit animation macro###EditAnimationName", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    GET_SESSION_MANAGER;

    if (!lateOpen && open) {
        const auto& animNames = sessionManager.getCurrentSession()->getAnimationNames();

        if (animNames.find(animationIndex) != animNames.end()) {
            const auto& str = animNames.at(animationIndex);
            strncpy(newMacro, str.c_str(), MACRO_BUF_SIZE - 1);
        }
        else {
            const std::string& str = sessionManager.getCurrentSession()->getCellanimName();
            snprintf(newMacro, MACRO_BUF_SIZE, "%.*s_", static_cast<int>(str.size() - 6), str.c_str());
        }
    }

    if (open) {
        ImGui::Text("Edit macro name for animation no. %u:", animationIndex + 1);
        ImGui::InputText("##Input", newMacro, MACRO_BUF_SIZE);

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            // Replace spaces with underscores
            {
                char* character = newMacro;

                while (*character != '\0') {
                    if (*character == ' ')
                        *character = '_';
                    character++;
                }
            }

            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandModifyAnimationName>(
                sessionManager.getCurrentSession()->currentCellanim,
                animationIndex,
                newMacro
            ));

            sessionManager.getCurrentSessionModified() = true;

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    lateOpen = open;
}

#endif // POPUP_EDITANIMATIONNAME
