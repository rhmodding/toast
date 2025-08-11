#include "SheetRepackFailed.hpp"

void Popups::SheetRepackFailed::Update() {
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

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