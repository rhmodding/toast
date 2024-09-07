#ifndef POPUP_ATTEMPTEXITWITHCHANGES
#define POPUP_ATTEMPTEXITWITHCHANGES

#include <imgui.h>

#include "../../App.hpp"
extern App* gAppPtr;

#include "../../common.hpp"

// AttemptExitWhileUnsavedChanges
void Popup_ExitWithChanges() {
    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal("Exit with unsaved changes###ExitWithChanges", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("There are still unsaved changes in one or more sessions.\nAre you sure you want to close the app?");

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Exit without saving")) {
            ImGui::CloseCurrentPopup();

            gAppPtr->AttemptExit(true);
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_ATTEMPTEXITWITHCHANGES
