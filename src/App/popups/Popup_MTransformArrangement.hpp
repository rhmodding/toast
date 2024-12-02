#ifndef POPUP_MTRANSFORMARRANGEMENT_HPP
#define POPUP_MTRANSFORMARRANGEMENT_HPP

#include <cstdint>
#include <cstring>

#include <imgui.h>

#include "../../anim/Animatable.hpp"

#include "../../AppState.hpp"

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangement.hpp"

void Popup_MTransformArrangement() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MTransformArrangement");

    GET_ANIMATABLE;

    RvlCellAnim::Arrangement* arrangement { nullptr };

    if (globalAnimatable.cellanim)
        arrangement = globalAnimatable.getCurrentArrangement();

    static const int noOffset[2] { 0, 0 };
    static const float noScale[2] { 1.f, 1.f };

    if (!active && lateOpen && arrangement) {
        memcpy(arrangement->tempOffset, noOffset, sizeof(int)*2);
        memcpy(arrangement->tempScale, noScale, sizeof(float)*2);

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
            memcpy(arrangement->tempOffset, noOffset, sizeof(int)*2);
            memcpy(arrangement->tempScale, noScale, sizeof(float)*2);

            auto newArrangement = *arrangement;
            for (auto& part : newArrangement.parts) {
                part.transform.positionX = static_cast<int16_t>(
                    (part.transform.positionX * scale[0]) + offset[0]
                );
                part.transform.positionY = static_cast<int16_t>(
                    (part.transform.positionY * scale[1]) + offset[1]
                );

                part.transform.scaleX *= scale[0];
                part.transform.scaleY *= scale[1];

                if (scale[0] < 0.f)
                    part.transform.angle = -part.transform.angle;
                if (scale[1] < 0.f)
                    part.transform.angle = -part.transform.angle;
            }

            GET_SESSION_MANAGER;

            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandModifyArrangement>(
                sessionManager.getCurrentSession()->currentCellanim,
                globalAnimatable.getCurrentKey()->arrangementIndex,
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

        memcpy(arrangement->tempOffset, offset, sizeof(int)*2);
        memcpy(arrangement->tempScale, scale, sizeof(float)*2);

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

#endif // POPUP_MTRANSFORMARRANGEMENT_HPP
