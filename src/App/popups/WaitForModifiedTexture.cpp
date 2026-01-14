#include "WaitForModifiedTexture.hpp"

#include <imgui.h>

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"

#include "manager/AppState.hpp"

#include "texture/TextureEx.hpp"

#include "command/CommandModifySpritesheet.hpp"

#include "Macro.hpp"

#include "ModifiedTextureSize.hpp"

void Popups::WaitForModifiedTexture::update() {
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

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

            std::shared_ptr<TextureEx> newTexture = std::make_shared<TextureEx>();
            if (newTexture->loadSTBFile(texturePath)) {
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

                Popups::ModifiedTextureSize::getInstance().setOldTextureSizes(cellanimSheet->getWidth(), cellanimSheet->getHeight());

                newTexture->setName(cellanimSheet->getName());

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifySpritesheet>(
                    sessionManager.getCurrentSession()
                        ->getCurrentCellAnim().object
                        ->getSheetIndex(),
                    newTexture
                ));

                ImGui::CloseCurrentPopup();

                if (diffSize)
                    Popups::ModifiedTextureSize::getInstance().open();
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