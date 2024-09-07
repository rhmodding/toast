#ifndef POPUP_WAITFORMODIFIEDTEXTURE
#define POPUP_WAITFORMODIFIEDTEXTURE

#include <imgui.h>

#include "../../SessionManager.hpp"
#include "../../ConfigManager.hpp"

#include "../../AppState.hpp"

#include "../../common.hpp"

void Popup_WaitForModifiedTexture() {
    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal("Modifying texture via image editor###WaitForModifiedTexture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "Once the modified texture is ready, select the \"Ready\" option.\n"
            "To cancel, select \"Nevermind\"."
        );

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        static const char* formatList[] = {
            "RGBA32 (full-depth color)",
            "RGB5A3 (variable color depth)",
        };
        static int selectedFormatIndex{ 0 };
        ImGui::Combo("Image Format", &selectedFormatIndex, formatList, ARRAY_LENGTH(formatList));

        ImGui::Dummy({ 0, 5 });

        if (ImGui::Button("Ready", ImVec2(120, 0))) {
            GET_SESSION_MANAGER;

            std::shared_ptr<Common::Image> newImage =
                std::make_shared<Common::Image>(Common::Image());
            newImage->LoadFromFile(ConfigManager::getInstance().getConfig().textureEditPath.c_str());

            if (newImage->texture) {
                auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

                bool diffSize =
                    newImage->width  != cellanimSheet->width ||
                    newImage->height != cellanimSheet->height;

                cellanimSheet = newImage;

                switch (selectedFormatIndex) {
                    case 0:
                        cellanimSheet->tplOutFormat = TPL::TPL_IMAGE_FORMAT_RGBA32;
                        break;

                    case 1:
                        cellanimSheet->tplOutFormat = TPL::TPL_IMAGE_FORMAT_RGB5A3;
                        break;

                    default:
                        break;
                }

                ImGui::CloseCurrentPopup();

                if (diffSize)
                    AppState::getInstance().OpenGlobalPopup("###DialogModifiedPNGSizeDiff");

                sessionManager.SessionChanged();
                sessionManager.getCurrentSessionModified() = true;
            }
            else
                ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_WAITFORMODIFIEDTEXTURE
