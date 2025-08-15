#include "MPadRegion.hpp"

#include <imgui.h>

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

#include "command/CommandModifyArrangementPart.hpp"

void Popups::MPadRegion::update() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MPadRegion");

    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    bool openConditions = sessionManager.getCurrentSessionIndex() >= 0;

    if (openConditions) {
        SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();
        openConditions &= selectionState.singlePartSelected();
    }

    static CellAnim::UintVec2 origCellOrigin;
    static CellAnim::UintVec2 origCellSize;

    static CellAnim::IntVec2 origPosition;

    if (!active && lateOpen && openConditions) {
        SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

        auto& part = playerManager.getArrangement().parts.at(selectionState.mSelectedParts[0].index);

        part.cellOrigin = origCellOrigin;
        part.cellSize = origCellSize;

        part.transform.position = origPosition;

        lateOpen = false;
    }

    if (active && openConditions) {
        SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

        auto& part = playerManager.getArrangement().parts.at(selectionState.mSelectedParts[0].index);

        if (!lateOpen) {
            origCellOrigin = part.cellOrigin;
            origCellSize = part.cellSize;

            origPosition = part.transform.position;
        }

        lateOpen = true;

        ImGui::SeparatorText("Part Cell Padding");

        static CellAnim::IntVec2 padBy { 0, 0 };
        static bool centerPart { true };

        ImGui::DragInt2("Padding (pixels)", padBy.asArray(), .5f);

        ImGui::Checkbox("Center part", &centerPart);

        ImGui::Separator();

        if (ImGui::Button("Apply")) {
            part.cellSize = origCellSize;
            part.cellOrigin = origCellOrigin;

            part.transform.position = origPosition;

            auto newPart = part; // Copy

            newPart.cellSize.x = origCellSize.x + padBy.x;
            newPart.cellSize.y = origCellSize.y + padBy.y;

            newPart.cellOrigin.x = origCellOrigin.x - (padBy.x / 2);
            newPart.cellOrigin.y = origCellOrigin.y - (padBy.y / 2);

            if (centerPart) {
                newPart.transform.position.x = origPosition.x - ((padBy.x / 2) * newPart.transform.scale.x);
                newPart.transform.position.y = origPosition.y - ((padBy.y / 2) * newPart.transform.scale.y);
            }
            else {
                newPart.transform.position.x = origPosition.x;
                newPart.transform.position.y = origPosition.y;
            }

            SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangementPart>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getArrangementIndex(),
                selectionState.mSelectedParts[0].index,
                newPart
            ));

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            padBy = { 0, 0 };

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        part.cellSize.x = origCellSize.x + padBy.x;
        part.cellSize.y = origCellSize.y + padBy.y;

        part.cellOrigin.x = origCellOrigin.x - (padBy.x / 2);
        part.cellOrigin.y = origCellOrigin.y - (padBy.y / 2);

        if (centerPart) {
            part.transform.position.x = origPosition.x - ((padBy.x / 2) * part.transform.scale.x);
            part.transform.position.y = origPosition.y - ((padBy.y / 2) * part.transform.scale.y);
        }
        else {
            part.transform.position.x = origPosition.x;
            part.transform.position.y = origPosition.y;
        }
    }

    if (active)
        ImGui::EndPopup();

    ImGui::PopStyleVar();
}
