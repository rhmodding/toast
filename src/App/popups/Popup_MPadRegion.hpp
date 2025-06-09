#ifndef POPUP_MPADREGION_HPP
#define POPUP_MPADREGION_HPP

#include <imgui.h>

#include "../../AppState.hpp"

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangementPart.hpp"

static void Popup_MPadRegion() {
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

    static unsigned origOffset[2] { 8, 8 };
    static unsigned origSize[2] { 32, 32 };

    static unsigned origPosition[2] { 0, 0 };

    if (!active && lateOpen && openConditions) {
        SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

        auto& part = playerManager.getArrangement().parts.at(selectionState.mSelectedParts[0].index);

        part.regionPos.x = origOffset[0];
        part.regionPos.y = origOffset[1];
        part.regionSize.x = origSize[0];
        part.regionSize.y = origSize[0];

        part.transform.position.x = origPosition[0];
        part.transform.position.y = origPosition[1];

        lateOpen = false;
    }

    if (active && openConditions) {
        SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

        auto& part = playerManager.getArrangement().parts.at(selectionState.mSelectedParts[0].index);

        if (!lateOpen) {
            origOffset[0] = part.regionPos.x;
            origOffset[1] = part.regionPos.y;

            origSize[0] = part.regionSize.x;
            origSize[1] = part.regionSize.y;

            origPosition[0] = part.transform.position.x;
            origPosition[1] = part.transform.position.y;
        }

        lateOpen = true;

        ImGui::SeparatorText("Part Region Padding");

        static int padBy[2] { 0, 0 };
        static bool centerPart { true };

        ImGui::DragInt2("Padding (pixels)", padBy, .5f);

        ImGui::Checkbox("Center part", &centerPart);

        ImGui::Separator();

        if (ImGui::Button("Apply")) {
            auto newPart = part; // Copy

            newPart.regionSize.x = origSize[0] + padBy[0];
            newPart.regionSize.y = origSize[1] + padBy[1];

            newPart.regionPos.x = origOffset[0] - (padBy[0] / 2);
            newPart.regionPos.y = origOffset[1] - (padBy[1] / 2);

            if (centerPart) {
                newPart.transform.position.x = origPosition[0] - (padBy[0] / 2);
                newPart.transform.position.y = origPosition[1] - (padBy[1] / 2);
            }

            part.regionPos.x = origOffset[0];
            part.regionPos.y = origOffset[1];

            part.regionSize.x = origSize[0];
            part.regionSize.y = origSize[1];

            part.transform.position.x = origPosition[0];
            part.transform.position.y = origPosition[1];

            SelectionState& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangementPart>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getArrangementIndex(),
                selectionState.mSelectedParts[0].index,
                newPart
            ));

            padBy[0] = 0;
            padBy[1] = 0;

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            padBy[0] = 0;
            padBy[1] = 0;

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        part.regionSize.x = origSize[0] + padBy[0];
        part.regionSize.y = origSize[1] + padBy[1];

        part.regionPos.x = origOffset[0] - (padBy[0] / 2);
        part.regionPos.y = origOffset[1] - (padBy[1] / 2);

        if (centerPart) {
            part.transform.position.x = origPosition[0] - (padBy[0] / 2);
            part.transform.position.y = origPosition[1] - (padBy[1] / 2);
        }
        else {
            part.transform.position.x = origPosition[0];
            part.transform.position.y = origPosition[1];
        }
    }

    if (active)
        ImGui::EndPopup();

    ImGui::PopStyleVar();
}

#endif // POPUP_MPADREGION_HPP
