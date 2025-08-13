#include "ModifiedTextureSize.hpp"

#include <imgui.h>

#include "manager/SessionManager.hpp"

#include "command/CommandModifyArrangements.hpp"

#include "Macro.hpp"

void Popups::ModifiedTextureSize::Update() {
    if (mOldTextureSizeX < 0 || mOldTextureSizeY < 0)
        return;

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    const bool active =
        ImGui::BeginPopupModal("Texture size mismatch###ModifiedTextureSize", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    if (active) {
        ImGui::TextUnformatted(
            "The new texture's size is different from the existing texture.\n"
            "Please select the desired behavior:"
        );

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("No cell scaling"))
            ImGui::CloseCurrentPopup();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip("Each part's cell will be kept as is (unscaled).");

        ImGui::SameLine();

        if (ImGui::Button("Apply cell scaling")) {
            SessionManager& sessionManager = SessionManager::getInstance();

            Session* currentSession = sessionManager.getCurrentSession();

            std::vector<CellAnim::Arrangement> newArrangements =
                currentSession->getCurrentCellAnim().object->getArrangements();

            float scaleX =
                static_cast<float>(currentSession->getCurrentCellAnimSheet()->getWidth()) /
                mOldTextureSizeX;
            float scaleY =
                static_cast<float>(currentSession->getCurrentCellAnimSheet()->getHeight()) /
                mOldTextureSizeY;

            for (auto& arrangement : newArrangements) {
                for (auto& part : arrangement.parts) {
                    part.cellOrigin.x *= scaleX;
                    part.cellSize.x *= scaleX;
                    part.cellOrigin.y *= scaleY;
                    part.cellSize.y *= scaleY;

                    part.transform.scale.x /= scaleX;
                    part.transform.scale.y /= scaleY;
                }
            }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangements>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                newArrangements
            ));

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip("Each part's cell will be scaled to match the new texture size.");

        ImGui::EndPopup();
    }
}