#ifndef POPUP_MPADREGION
#define POPUP_MPADREGION

#include <cstdint>

#include <imgui.h>

#include "../../anim/Animatable.hpp"

#include "../../AppState.hpp"

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangementPart.hpp"

void Popup_MPadRegion() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MPadRegion");

    GET_APP_STATE;
    GET_ANIMATABLE;

    RvlCellAnim::ArrangementPart* part { nullptr };

    if (
        globalAnimatable && globalAnimatable->cellanim &&
        appState.selectedPart != -1
    )
        part = &globalAnimatable->getCurrentArrangement()->parts.at(appState.selectedPart);

    static uint16_t origOffset[2] { 8, 8 };
    static uint16_t origSize[2] { 32, 32 };

    static int16_t origPosition[2] { CANVAS_ORIGIN, CANVAS_ORIGIN };

    if (!active && lateOpen && part) {
        part->regionX = origOffset[0];
        part->regionY = origOffset[1];

        part->regionW = origSize[0];
        part->regionH = origSize[0];

        part->transform.positionX = origPosition[0];
        part->transform.positionY = origPosition[1];

        lateOpen = false;
    }

    if (active && part) {
        if (!lateOpen) {
            origOffset[0] = part->regionX;
            origOffset[1] = part->regionY;

            origSize[0] = part->regionW;
            origSize[1] = part->regionH;

            origPosition[0] = part->transform.positionX;
            origPosition[1] = part->transform.positionY;
        }

        lateOpen = true;

        ImGui::SeparatorText("Part Region Padding");

        static int padBy[2] { 0, 0 };
        static bool centerPart { true };

        ImGui::DragInt2("Padding (pixels)", padBy, .5f);

        ImGui::Checkbox("Center part", &centerPart);

        ImGui::Separator();

        if (ImGui::Button("Apply")) {
            auto newPart = *part;

            newPart.regionW = origSize[0] + padBy[0];
            newPart.regionH = origSize[1] + padBy[1];

            newPart.regionX = origOffset[0] - (padBy[0] / 2);
            newPart.regionY = origOffset[1] - (padBy[1] / 2);

            if (centerPart) {
                newPart.transform.positionX = origPosition[0] - (padBy[0] / 2);
                newPart.transform.positionY = origPosition[1] - (padBy[1] / 2);
            }

            part->regionX = origOffset[0];
            part->regionY = origOffset[1];

            part->regionW = origSize[0];
            part->regionH = origSize[1];

            part->transform.positionX = origPosition[0];
            part->transform.positionY = origPosition[1];

            GET_SESSION_MANAGER;

            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandModifyArrangementPart>(
                sessionManager.getCurrentSession()->currentCellanim,
                globalAnimatable->getCurrentKey()->arrangementIndex,
                appState.selectedPart,
                newPart
            ));

            sessionManager.getCurrentSessionModified() = true;

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

        part->regionW = origSize[0] + padBy[0];
        part->regionH = origSize[1] + padBy[1];

        part->regionX = origOffset[0] - (padBy[0] / 2);
        part->regionY = origOffset[1] - (padBy[1] / 2);

        if (centerPart) {
            part->transform.positionX = origPosition[0] - (padBy[0] / 2);
            part->transform.positionY = origPosition[1] - (padBy[1] / 2);
        }
        else {
            part->transform.positionX = origPosition[0];
            part->transform.positionY = origPosition[1];
        }
    }

    if (active)
        ImGui::EndPopup();

    ImGui::PopStyleVar();
}

#endif // POPUP_MPADREGION
