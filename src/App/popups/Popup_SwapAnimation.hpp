#ifndef POPUP_SWAPANIMATION_HPP
#define POPUP_SWAPANIMATION_HPP

#include <imgui.h>

#include <sstream>

#include <cmath>

#include <algorithm>

#include <limits>

#include "../../SessionManager.hpp"
#include "../../command/CommandSwapAnimations.hpp"

#include "../../AppState.hpp"

#include "../../Easings.hpp"

#include "../../common.hpp"

static void Popup_SwapAnimation(int animationIndex) {
    if (animationIndex < 0)
        return;

    CENTER_NEXT_WINDOW();

    if (ImGui::BeginPopupModal("Swap animations###SwapAnimation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static float animationBegin { static_cast<float>(ImGui::GetTime()) };
        static const float animationTime { .25f };

        static bool swapNames { true };

        static int swapAnim { -1 };

        // Left
        {
            ImGui::BeginGroup();

            ImGui::SetNextWindowSizeConstraints(
                { 200.f, 380.f },
                {
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()
                }
            );
            if (ImGui::BeginChild(
                "Properties",
                { 0.f, -ImGui::GetFrameHeightWithSpacing() },
                ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX
            )) {
                if (swapAnim < 0) {
                    ImGui::Bullet();
                    ImGui::SameLine();
                    ImGui::TextWrapped("Select a animation to swap with.");
                }
                else {
                    ImGui::Checkbox("Swap macro names", &swapNames);
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();

            ImGui::BeginDisabled(swapAnim < 0);
            if (ImGui::Button("Apply")) {
                SessionManager& sessionManager = SessionManager::getInstance();

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandSwapAnimations>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    animationIndex,
                    swapAnim,
                    swapNames
                ));

                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::EndGroup();
        }

        ImGui::SameLine();

        // Right
        {
            ImGui::SetNextWindowSizeConstraints(
                { 400.f, 380.f },
                {
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()
                }
            );
            ImGui::BeginChild("Visualization", { 0.f, 0.f }, ImGuiChildFlags_Border);

            float beginCursorY = ImGui::GetCursorPosY();

            Animatable& globalAnimatable = AppState::getInstance().globalAnimatable;

            for (int n = 0; n < (int)globalAnimatable.cellanim->animations.size(); n++) {
                std::ostringstream fmtStream;

                SessionManager& sessionManager = SessionManager::getInstance();

                int nI = swapAnim != -1 ? n == animationIndex ? swapAnim : n == swapAnim ? animationIndex : n : n;

                const char* animName =
                    sessionManager.getCurrentSession()->getCurrentCellanim().object
                        ->animations.at(swapNames ? n : nI).name.c_str();
                if (animName == nullptr)
                    animName = "(no macro defined)";

                fmtStream << std::to_string(nI+1) << ". " << animName;
                if (nI != n)
                    fmtStream << " (old index: " << n+1 << ")";

                float positionOrig = beginCursorY + (n * (
                    ImGui::CalcTextSize(fmtStream.str().c_str(), nullptr, true).y +
                    (ImGui::GetStyle().ItemSpacing.y)
                ));
                float positionNew = beginCursorY + (nI * (
                    ImGui::CalcTextSize(fmtStream.str().c_str(), nullptr, true).y +
                    (ImGui::GetStyle().ItemSpacing.y)
                ));

                float animProgression = std::clamp<float>((ImGui::GetTime() - animationBegin) / animationTime, 0.f, 1.f);
                ImGui::SetCursorPosY(std::lerp(positionOrig, positionNew, Easings::InOut(animProgression)));

                if (ImGui::Selectable(fmtStream.str().c_str(), animationIndex == n || swapAnim == n, ImGuiSelectableFlags_DontClosePopups)) {
                    swapAnim = n;

                    animationBegin = ImGui::GetTime();
                }
            }

            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }
}

#endif // POPUP_SWAPANIMATION_HPP
