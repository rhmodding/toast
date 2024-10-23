#ifndef POPUP_EDITORDATAEXPECTED
#define POPUP_EDITORDATAEXPECTED

#include <imgui.h>

#include "../../common.hpp"

void Popup_EditorDataExpected() {
    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Data not found###EditorDataExpected", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "The supplemental editor data for this file (TOAST.DAT) is missing.\n"
            "Data such as part visibillity and locking has been lost."
        );

        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Alright", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_EDITORDATAEXPECTED
