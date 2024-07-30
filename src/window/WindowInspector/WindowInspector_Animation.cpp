#include "../WindowInspector.hpp"

#include "../../font/FontAwesome.h"

#include "../../SessionManager.hpp"

#include "../../AppState.hpp"
#include "../../anim/Animatable.hpp"

#include "../../command/CommandModifyAnimationName.hpp"
#include "../../command/CommandSwapAnimations.hpp"

#include <sstream>

void WindowInspector::Level_Animation() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    this->DrawPreview(globalAnimatable);

    ImGui::SameLine();

    uint16_t animationIndex = globalAnimatable->getCurrentAnimationIndex();
    auto query = sessionManager.getCurrentSession()->getAnimationNames().find(animationIndex);

    const char* animationName =
        query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
            query->second.c_str() : nullptr;

    ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

        ImGui::Text("Animation no. %u:", animationIndex + 1);

        ImGui::PushFont(appState.fonts.large);
        ImGui::TextWrapped("%s", animationName ? animationName : "(no macro defined)");
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();


    RvlCellAnim::Animation* animation = globalAnimatable->getCurrentAnimation();
    static char newMacroName[256]{ "" };

    static int32_t swapAnim{ -1 };

    ImGui::SeparatorText((char*)ICON_FA_PENCIL " Properties");
    if (ImGui::Button("Edit macro name..")) {
        std::string copyName(animationName ? animationName : "");
        if (copyName.size() > 255)
            copyName.resize(255); // Truncate

        strcpy(newMacroName, copyName.c_str());
        ImGui::OpenPopup("###EditMacroNamePopup");
    }
    ImGui::SameLine();
    if (ImGui::Button("Swap index..")) {
        swapAnim = -1;
        ImGui::OpenPopup("###SwapIndexPopup");
    }

    ImGui::Indent();
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextWrapped("Macro names are not used for mapping animations directly in the game.\nTo swap animations, use [Swap index..].");
    ImGui::Unindent();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal("Edit macro name###EditMacroNamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Edit macro name for animation no. %u:", animationIndex);
        ImGui::InputText("##MacroNameInput", newMacroName, 256);

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            changed = true;

            // Replace spaces with underscores
            {
                char* character = newMacroName;

                while (*character != '\0') {
                    if (*character == ' ')
                        *character = '_';
                    character++;
                }
            }

            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandModifyAnimationName>(
                sessionManager.getCurrentSession()->currentCellanim,
                animationIndex,
                newMacroName
            ));

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Swap animation index###SwapIndexPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static float animationBegin{ static_cast<float>(ImGui::GetTime()) };
        const static float animationTime{ .25f };

        static bool swapNames{ true };

        // Left
        {
            ImGui::BeginGroup();
            if (ImGui::BeginChild("Properties", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX)) {
                if (swapAnim == -1) {
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

            ImGui::BeginDisabled(swapAnim == -1);
            if (ImGui::Button("Apply")) {
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandSwapAnimations>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    animationIndex,
                    swapAnim,
                    swapNames
                ));

                changed = true;

                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::EndGroup();
        }

        ImGui::SameLine();

        // Right
        {
            ImGui::SetNextWindowSizeConstraints({ 400.f, 380.f }, { FLT_MAX, FLT_MAX });
            ImGui::BeginChild("Visualization", { 0, 0 }, ImGuiChildFlags_Border);

            float beginCursorY = ImGui::GetCursorPosY();

            for (uint16_t n = 0; n < globalAnimatable->cellanim->animations.size(); n++) {
                std::ostringstream fmtStream;

                GET_SESSION_MANAGER;

                uint16_t nI = swapAnim != -1 ? n == animationIndex ? swapAnim : n == swapAnim ? animationIndex : n : n;

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
