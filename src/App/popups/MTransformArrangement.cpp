#include "MTransformArrangement.hpp"

#include <imgui.h>

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"

#include "command/CommandModifyArrangement.hpp"

#include "Macro.hpp"

void Popups::MTransformArrangement::Update() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MTransformArrangement");

    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    CellAnim::Arrangement* arrangement { nullptr };

    if (sessionManager.getCurrentSessionIndex() >= 0)
        arrangement = &playerManager.getArrangement();


    if (!active && lateOpen && arrangement) {
        arrangement->tempOffset = { 0, 0 };
        arrangement->tempScale = { 1.f, 1.f };

        lateOpen = false;
    }

    if (active) {
        lateOpen = true;

        ImGui::SeparatorText("Transform Arrangement");

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
            arrangement->tempOffset = { 0, 0 };
            arrangement->tempScale = { 1.f, 1.f };

            auto newArrangement = *arrangement;
            for (auto& part : newArrangement.parts) {
                part.transform.position.x = static_cast<int16_t>(
                    (part.transform.position.x * scale.x) + offset.x
                );
                part.transform.position.y = static_cast<int16_t>(
                    (part.transform.position.y * scale.y) + offset.y
                );

                part.transform.scale *= scale;

                if (scale.x < 0.f)
                    part.transform.angle = -part.transform.angle;
                if (scale.y < 0.f)
                    part.transform.angle = -part.transform.angle;
            }

            SessionManager& sessionManager = SessionManager::getInstance();

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangement>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getArrangementIndex(),
                newArrangement
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

        arrangement->tempOffset = offset;
        arrangement->tempScale = scale;

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}