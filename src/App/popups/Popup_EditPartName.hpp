#ifndef POPUP_EDITPARTNAME_HPP
#define POPUP_EDITPARTNAME_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangementPart.hpp"

#include "../../common.hpp"

static void Popup_EditPartName(int arrangementIndex, int partIndex) {
    if (arrangementIndex < 0 || partIndex < 0)
        return;

    static bool lateOpen { false };

    static char newName[sizeof(RvlCellAnim::ArrangementPart::editorName)] { '\0' };

    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    bool open = ImGui::BeginPopupModal("Edit part editor name###EditPartName", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    SessionManager& sessionManager = SessionManager::getInstance();

    if (!lateOpen && open) {
        const RvlCellAnim::ArrangementPart& part =
            sessionManager.getCurrentSession()
                ->getCurrentCellanim().object
                ->arrangements.at(arrangementIndex).parts.at(partIndex);

        if (part.editorName[0] != '\0')
            strncpy(newName, part.editorName, sizeof(newName) - 1);
        else
            newName[0] = '\0';
    }

    if (open) {
        ImGui::Text("Edit name for part no. %u (arrangement no. %u):", partIndex + 1, arrangementIndex + 1);
        ImGui::InputText("##Input", newName, sizeof(newName));

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("OK", { 120.f, 0.f })) {
            RvlCellAnim::ArrangementPart newPart =
                sessionManager.getCurrentSession()
                ->getCurrentCellanim().object
                ->arrangements.at(arrangementIndex).parts.at(partIndex);

            strncpy(newPart.editorName, newName, sizeof(newPart.editorName) - 1);

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangementPart>(
                sessionManager.getCurrentSession()->currentCellanim,
                arrangementIndex, partIndex,
                newPart
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

#endif // POPUP_EDITPARTNAME_HPP
