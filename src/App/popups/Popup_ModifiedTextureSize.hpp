#ifndef POPUP_MODIFIEDTEXTURESIZE
#define POPUP_MODIFIEDTEXTURESIZE

#include <imgui.h>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangements.hpp"

#include "../../common.hpp"

void Popup_ModifiedTextureSize() {
    CENTER_NEXT_WINDOW;

    // TODO: allow modified image undo

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal("Texture size mismatch###DialogModifiedPNGSizeDiff", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("The new texture's size is different from the existing texture.\nPlease select the wanted behavior:");

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        ImGui::PopStyleVar();

        if (ImGui::Button("No region scaling")) {
            SessionManager::Session* currentSession = SessionManager::getInstance().getCurrentSession();

            currentSession->getCellanimObject()->textureW = currentSession->getCellanimSheet()->width;
            currentSession->getCellanimObject()->textureH = currentSession->getCellanimSheet()->height;

            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip("Select this option for texture extensions.");

        ImGui::SameLine();

        if (ImGui::Button("Apply region scaling")) {
            GET_SESSION_MANAGER;

            SessionManager::Session* currentSession = sessionManager.getCurrentSession();

            std::vector<RvlCellAnim::Arrangement> newArrangements =
                currentSession->getCellanimObject()->arrangements;

            float scaleX =
                static_cast<float>(currentSession->getCellanimSheet()->width) /
                currentSession->getCellanimObject()->textureW;
            float scaleY =
                static_cast<float>(currentSession->getCellanimSheet()->height) /
                currentSession->getCellanimObject()->textureH;

            for (auto& arrangement : newArrangements)
                for (auto& part : arrangement.parts) {
                    part.regionX *= scaleX;
                    part.regionW *= scaleX;
                    part.regionY *= scaleY;
                    part.regionH *= scaleY;

                    part.transform.scaleX /= scaleX;
                    part.transform.scaleY /= scaleY;
                }

            currentSession->getCellanimObject()->textureW = currentSession->getCellanimSheet()->width;
            currentSession->getCellanimObject()->textureH = currentSession->getCellanimSheet()->height;

            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandModifyArrangements>(
                sessionManager.getCurrentSession()->currentCellanim,
                newArrangements
            ));

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip("Select this option for texture upscales.");

        ImGui::EndPopup();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 }); // sigh
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_MODIFIEDTEXTURESIZE
