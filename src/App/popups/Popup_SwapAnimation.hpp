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

#include "../../common.hpp"

static void Popup_SwapAnimation(int animationIndex) {
    if (animationIndex < 0)
        return;

    CENTER_NEXT_WINDOW;

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
                GET_SESSION_MANAGER;

                sessionManager.getCurrentSession()->executeCommand(
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

            GET_ANIMATABLE;

            for (int n = 0; n < (int)globalAnimatable.cellanim->animations.size(); n++) {
                std::ostringstream fmtStream;

                GET_SESSION_MANAGER;

                int nI = swapAnim != -1 ? n == animationIndex ? swapAnim : n == swapAnim ? animationIndex : n : n;

                auto query = sessionManager.getCurrentSession()->getAnimationNames().find(swapNames ? n : nI);

                const char* animName =
                    query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
                        query->second.c_str() :
                        "(no macro defined)";

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
                ImGui::SetCursorPosY(std::lerp(positionOrig, positionNew, Common::EaseInOut(animProgression)));

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
