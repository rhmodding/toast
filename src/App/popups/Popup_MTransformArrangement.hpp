#ifndef POPUP_MTRANSFORMARRANGEMENT_HPP
#define POPUP_MTRANSFORMARRANGEMENT_HPP

#include <cstdint>
#include <cstring>

#include <imgui.h>

#include "manager/SessionManager.hpp"

#include "command/CommandModifyArrangement.hpp"

static void Popup_MTransformArrangement() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MTransformArrangement");

    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    CellAnim::Arrangement* arrangement { nullptr };

    if (sessionManager.getCurrentSessionIndex() >= 0)
        arrangement = &playerManager.getArrangement();

    static const int noOffset[2] { 0, 0 };
    static const float noScale[2] { 1.f, 1.f };

    if (!active && lateOpen && arrangement) {
        memcpy(arrangement->tempOffset, noOffset, sizeof(arrangement->tempOffset));
        memcpy(arrangement->tempScale, noScale, sizeof(arrangement->tempScale));

        lateOpen = false;
    }

    if (active) {
        lateOpen = true;

        ImGui::SeparatorText("Transform Arrangement");

        static int offset[2] { 0, 0 };
        static float scale[2] { 1.f, 1.f };

        static bool uniformScale { true };

        ImGui::DragInt2("Offset XY", offset, 1.f);

        const float scaleBefore[2] = { scale[0], scale[1] };
        if (ImGui::DragFloat2("Scale XY", scale, .01f) && uniformScale) {
            if (scale[0] != scaleBefore[0])
                scale[1] = scale[0];
            if (scale[1] != scaleBefore[1])
                scale[0] = scale[1];
        }

        if (ImGui::Checkbox("Uniform Scale", &uniformScale)) {
            if (scale[0] != scale[1]) {
                float v = AVERAGE_FLOATS(scale[0], scale[1]);

                scale[0] = v;
                scale[1] = v;
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Apply")) {
            memcpy(arrangement->tempOffset, noOffset, sizeof(arrangement->tempOffset));
            memcpy(arrangement->tempScale, noScale, sizeof(arrangement->tempScale));

            auto newArrangement = *arrangement;
            for (auto& part : newArrangement.parts) {
                part.transform.position.x = static_cast<int16_t>(
                    (part.transform.position.x * scale[0]) + offset[0]
                );
                part.transform.position.y = static_cast<int16_t>(
                    (part.transform.position.y * scale[1]) + offset[1]
                );

                part.transform.scale.x *= scale[0];
                part.transform.scale.y *= scale[1];

                if (scale[0] < 0.f)
                    part.transform.angle = -part.transform.angle;
                if (scale[1] < 0.f)
                    part.transform.angle = -part.transform.angle;
            }

            SessionManager& sessionManager = SessionManager::getInstance();

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangement>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getArrangementIndex(),
                newArrangement
            ));

            offset[0] = 0; offset[1] = 0;
            scale[0] = 1.f; scale[1] = 1.f;

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            offset[0] = 0; offset[1] = 0;
            scale[0] = 1.f; scale[1] = 1.f;

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        memcpy(arrangement->tempOffset, offset, sizeof(arrangement->tempOffset));
        memcpy(arrangement->tempScale, scale, sizeof(arrangement->tempScale));

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

#endif // POPUP_MTRANSFORMARRANGEMENT_HPP
