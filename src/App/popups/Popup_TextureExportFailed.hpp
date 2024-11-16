#ifndef POPUP_TEXTUREEXPORTFAILED_HPP
#define POPUP_TEXTUREEXPORTFAILED_HPP

#include <imgui.h>

#include "../../common.hpp"

void Popup_TextureExportFailed() {
    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("There was an error exporting the texture.###TextureExportFailed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "The texture could not be written to the PNG file.\n"
            "Please check the path set in the config."
        );

        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Alright", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_TEXTUREEXPORTFAILED_HPP
