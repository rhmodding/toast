#ifndef POPUP_MOPTIMIZEGLOBAL_HPP
#define POPUP_MOPTIMIZEGLOBAL_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../task/AsyncTaskManager.hpp"
#include "../../task/AsyncTaskOptimizeCellanim.hpp"

static void Popup_MOptimizeGlobal() {
    static OptimizeCellanimOptions options {};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Optimize Cellanim###MOptimizeGlobal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        SessionManager& sessionManager = SessionManager::getInstance();

        ImGui::TextUnformatted("Optimize the current Cellanim:");
        ImGui::BulletText("%u. \"%s\"",
            sessionManager.getCurrentSession()->getCurrentCellanimIndex() + 1,
            sessionManager.getCurrentSession()->getCurrentCellanim().name.c_str()
        );

        ImGui::Dummy({ 0.f, 5.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 10.f });

        static const char* downscaleComboItems[] {
            "Don't downscale (1x)",
            "Downscale Low (0.875x)",
            "Downscale Medium (0.75x)",
            "Downscale High (0.5x)",
        };

        // Animation names are required on CTR, doesn't make sense to remove them as
        // they are not supplemental.
        if (sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL)
            ImGui::Checkbox("Remove all animation names", &options.removeAnimationNames);
        // Make sure it's off..
        else
            options.removeAnimationNames = false;

        ImGui::Checkbox("Remove all unused arrangements", &options.removeUnusedArrangements);

        ImGui::Dummy({ 0.f, 5.f });

        ImGui::PopStyleVar();
        ImGui::Combo(
            "Downscale Spritesheet",
            reinterpret_cast<int*>(&options.downscaleSpritesheet),
            downscaleComboItems, ARRAY_LENGTH(downscaleComboItems)
        );
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });

        ImGui::Dummy({ 0.f, 12.f });

        ImGui::TextUnformatted(
            "This action is irreversible; You won't be able to undo this\n"
            "action or any action before it."
        );

        ImGui::Dummy({ 0.f, 3.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("OK", { 120.f, 0.f })) {
            ImGui::CloseCurrentPopup();

            AsyncTaskManager::getInstance().StartTask<AsyncTaskOptimizeCellanim>(
                sessionManager.getCurrentSession(), options
            );
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_MOPTIMIZEGLOBAL_HPP
