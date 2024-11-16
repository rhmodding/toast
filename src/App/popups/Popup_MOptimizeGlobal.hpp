#ifndef POPUP_MOPTIMIZEGLOBAL_HPP
#define POPUP_MOPTIMIZEGLOBAL_HPP

#include <imgui.h>

#include "../../AppState.hpp"

#include "../../SessionManager.hpp"

#include "../../task/AsyncTaskManager.hpp"
#include "../../task/AsyncTask_OptimizeCellanim.hpp"

void Popup_MOptimizeGlobal() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Optimize Cellanim###MOptimizeGlobal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        GET_SESSION_MANAGER;

        ImGui::TextUnformatted("Optimize the current Cellanim:");
        ImGui::BulletText("%u. \"%s\"",
            sessionManager.getCurrentSession()->currentCellanim + 1,
            sessionManager.getCurrentSession()->getCellanimName().c_str()
        );

        sessionManager.getCurrentSession()->getCellanimObject();

        ImGui::Dummy({ 0.f, 5.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 10.f });

        static OptimizeCellanimOptions options;

        static const char* downscaleComboItems[] {
            "Don't downscale (1x)",
            "Downscale Low (0.875x)",
            "Downscale Medium (0.75x)",
            "Downscale High (0.5x)",
        };
        static int downscaleOption { 0 };

        ImGui::Checkbox("Remove all animation name macros", &options.removeAnimationNames);
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

            AsyncTaskManager::getInstance().StartTask<OptimizeCellanimTask>(
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
