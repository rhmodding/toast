#ifndef POPUP_TEXTUREEXPORTFAILED
#define POPUP_TEXTUREEXPORTFAILED

#include <imgui.h>

#include "../../common.hpp"

void Popup_TextureExportFailed() {
    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal("There was an error exporting the texture.###TextureExportFailed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "The texture could not be written to the PNG file.\n"
            "Please check the path set in the config."
        );

        ImGui::Dummy({0, 5});

        if (ImGui::Button("Alright", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_TEXTUREEXPORTFAILED
