#ifndef POPUP_MODIFIEDTEXTURESIZE_HPP
#define POPUP_MODIFIEDTEXTURESIZE_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangements.hpp"

#include "../../common.hpp"

static void Popup_ModifiedTextureSize(int oldTextureSizeX, int oldTextureSizeY) {
    if (oldTextureSizeX < 0 || oldTextureSizeY < 0)
        return;
    
    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    const bool active =
        ImGui::BeginPopupModal("Texture size mismatch###ModifiedTextureSize", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    if (active) {
        ImGui::TextUnformatted("The new texture's size is different from the existing texture.\nPlease select the wanted behavior:");

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("No region scaling"))
            ImGui::CloseCurrentPopup();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip("Select this option for texture extensions.");

        ImGui::SameLine();

        if (ImGui::Button("Apply region scaling")) {
            GET_SESSION_MANAGER;

            Session* currentSession = sessionManager.getCurrentSession();

            std::vector<RvlCellAnim::Arrangement> newArrangements =
                currentSession->getCurrentCellanim().object->arrangements;

            float scaleX =
                static_cast<float>(currentSession->getCurrentCellanimSheet()->getWidth()) /
                oldTextureSizeX;
            float scaleY =
                static_cast<float>(currentSession->getCurrentCellanimSheet()->getHeight()) /
                oldTextureSizeY;

            for (auto& arrangement : newArrangements)
                for (auto& part : arrangement.parts) {
                    part.regionX *= scaleX;
                    part.regionW *= scaleX;
                    part.regionY *= scaleY;
                    part.regionH *= scaleY;

                    part.transform.scaleX /= scaleX;
                    part.transform.scaleY /= scaleY;
                }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangements>(
                sessionManager.getCurrentSession()->currentCellanim,
                newArrangements
            ));

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip("Select this option for texture upscales.");

        ImGui::EndPopup();
    }
}

#endif // POPUP_MODIFIEDTEXTURESIZE_HPP
