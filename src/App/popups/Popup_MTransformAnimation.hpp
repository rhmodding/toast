#ifndef POPUP_MTRANSFORMANIMATION_HPP
#define POPUP_MTRANSFORMANIMATION_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../command/CommandModifyAnimation.hpp"

static void Popup_MTransformAnimation() {
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    if (!sessionManager.getSessionAvaliable())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MTransformAnimation");

    if (!active && lateOpen)
        lateOpen = false;

    if (active) {
        lateOpen = true;

        ImGui::SeparatorText("Transform Animation");

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
            auto animation = playerManager.getAnimation();

            for (auto& key : animation.keys) {
                key.transform.positionX += offset[0];
                key.transform.positionY += offset[1];

                key.transform.scaleX *= scale[0];
                key.transform.scaleY *= scale[1];
            }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyAnimation>(
                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                playerManager.getAnimationIndex(),
                animation
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

#endif // POPUP_MTRANSFORMANIMATION_HPP
