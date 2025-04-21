#ifndef POPUP_WAITFORMODIFIEDTEXTURE_HPP
#define POPUP_WAITFORMODIFIEDTEXTURE_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"
#include "../../ConfigManager.hpp"

#include "../../AppState.hpp"

#include "../../command/CommandModifySpritesheet.hpp"

#include "../../common.hpp"

#include "../Popups.hpp"

static void Popup_WaitForModifiedTexture() {
    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Modifying texture via image editor###WaitForModifiedTexture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "Once the modified texture is ready, select the \"Ready\" option.\n"
            "To cancel, select \"Nevermind\"."
        );

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        static const char* formatList[] {
            "RGBA32 (full-depth color)",
            "RGB5A3 (variable color depth)",
        };
        static int selectedFormatIndex { 0 };
        ImGui::Combo("Image Format", &selectedFormatIndex, formatList, ARRAY_LENGTH(formatList));

        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Ready", { 120.f, 0.f })) {
            SessionManager& sessionManager = SessionManager::getInstance();

            const char* texturePath = ConfigManager::getInstance().getConfig().textureEditPath.c_str();

            std::shared_ptr<Texture> newTexture = std::make_shared<Texture>();
            bool ok = newTexture->LoadSTBFile(texturePath);

            if (ok) {
                auto& cellanimSheet = sessionManager.getCurrentSession()
                    ->getCurrentCellAnimSheet();
                switch (selectedFormatIndex) {
                default:
                case 0:
                    cellanimSheet->setTPLOutputFormat(TPL::TPL_IMAGE_FORMAT_RGBA32);
                    break;
                case 1:
                    cellanimSheet->setTPLOutputFormat(TPL::TPL_IMAGE_FORMAT_RGB5A3);
                    break;
                }

                bool diffSize =
                    newTexture->getWidth()  != cellanimSheet->getWidth() ||
                    newTexture->getHeight() != cellanimSheet->getHeight();

                Popups::_oldTextureSizeX = cellanimSheet->getWidth();
                Popups::_oldTextureSizeY = cellanimSheet->getHeight();

                newTexture->setName(cellanimSheet->getName());

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifySpritesheet>(
                    sessionManager.getCurrentSession()
                        ->getCurrentCellAnim().object
                        ->sheetIndex,
                    newTexture
                ));

                ImGui::CloseCurrentPopup();

                if (diffSize)
                    OPEN_GLOBAL_POPUP("###ModifiedTextureSize");
            }
            else
                ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_WAITFORMODIFIEDTEXTURE_HPP
