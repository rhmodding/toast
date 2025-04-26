#ifndef POPUP_ATTEMPTEXITWITHCHANGES_HPP
#define POPUP_ATTEMPTEXITWITHCHANGES_HPP

#include <imgui.h>

#include "../../Toast.hpp"

#include "../../common.hpp"

static void Popup_ExitWithChanges() {
    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Exit with unsaved changes###ExitWithChanges", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("There are still unsaved changes in one or more sessions.\nAre you sure you want to close the app?");

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Exit without saving")) {
            ImGui::CloseCurrentPopup();

            Toast::getInstance()->AttemptExit(true);
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_ATTEMPTEXITWITHCHANGES_HPP
