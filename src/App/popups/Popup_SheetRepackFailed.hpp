#ifndef POPUP_SHEETREPACKFAILED_HPP
#define POPUP_SHEETREPACKFAILED_HPP

#include <imgui.h>

#include "Macro.hpp"

static void Popup_SheetRepackFailed() {
    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("The spritesheet could not be repacked.###SheetRepackFailed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "The spritesheet could not not be repacked;\n"
            "no changes have been made."
        );

        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Alright", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_SHEETREPACKFAILED_HPP
