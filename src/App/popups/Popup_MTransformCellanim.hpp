#ifndef POPUP_MTRANSFORMCELLANIM_HPP
#define POPUP_MTRANSFORMCELLANIM_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../command/CommandModifyAnimations.hpp"

static void Popup_MTransformCellanim() {
    SessionManager& sessionManager = SessionManager::getInstance();

    if (!sessionManager.getSessionAvaliable())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MTransformCellanim");

    if (!active && lateOpen)
        lateOpen = false;

    if (active) {
        lateOpen = true;

        ImGui::SeparatorText("Transform Cellanim");

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
            auto newAnimations = sessionManager.getCurrentSession()
                ->getCurrentCellAnim().object->animations;

            for (auto& animation : newAnimations) {
                for (auto& key : animation.keys) {
                    key.transform.positionX += offset[0];
                    key.transform.positionY += offset[1];

                    key.transform.scaleX *= scale[0];
                    key.transform.scaleY *= scale[1];
                }
            }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyAnimations>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                newAnimations
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

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

#endif // POPUP_MTRANSFORMCELLANIM_HPP
