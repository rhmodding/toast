#include "MTransformAnimation.hpp"

#include <imgui.h>

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"

#include "command/CommandModifyAnimation.hpp"

#include "Macro.hpp"

void Popups::MTransformAnimation::update() {
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    if (!sessionManager.anySessionOpened())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MTransformAnimation");

    if (!active && lateOpen)
        lateOpen = false;

    if (active) {
        lateOpen = true;

        ImGui::SeparatorText("Transform Animation");

        static CellAnim::IntVec2 offset { 0, 0 };
        static CellAnim::FltVec2 scale { 1.f, 1.f };

        static bool uniformScale { true };

        ImGui::DragInt2("Offset XY", offset.asArray(), 1.f);

        CellAnim::FltVec2 scaleBefore = scale;
        if (ImGui::DragFloat2("Scale XY", scale.asArray(), .01f) && uniformScale) {
            if (scale.x != scaleBefore.x)
                scale.y = scale.x;
            if (scale.y != scaleBefore.y)
                scale.x = scale.y;
        }

        if (ImGui::Checkbox("Uniform Scale", &uniformScale)) {
            if (scale.x != scale.y) {
                float v = AVERAGE_FLOATS(scale.x, scale.y);
                scale = { v, v };
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Apply")) {
            auto animation = playerManager.getAnimation();

            for (auto& key : animation.keys) {
                key.transform.position += offset;
                key.transform.scale *= scale;
            }

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyAnimation>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getAnimationIndex(),
                animation
            ));

            offset = { 0, 0 };
            scale = { 1.f, 1.f };

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            offset = { 0, 0 };
            scale = { 1.f, 1.f };

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}