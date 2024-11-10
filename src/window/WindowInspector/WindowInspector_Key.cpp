#include "../WindowInspector.hpp"

#include "../../SessionManager.hpp"

#include "../../anim/Animatable.hpp"

#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../anim/CellanimHelpers.hpp"

#include "../../font/FontAwesome.h"

#include "../../AppState.hpp"

const uint16_t uint16_one = 1;

void WindowInspector::Level_Key() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    DrawPreview(globalAnimatable);

    ImGui::SameLine();

    RvlCellAnim::AnimationKey newKey = *globalAnimatable->getCurrentKey();
    RvlCellAnim::AnimationKey originalKey = *globalAnimatable->getCurrentKey();

    uint16_t animationIndex = globalAnimatable->getCurrentAnimationIndex();
    auto query = sessionManager.getCurrentSession()->getAnimationNames().find(animationIndex);

    const char* animationName =
        query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
            query->second.c_str() : nullptr;

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Anim \"%s\" (no. %u)", animationName ? animationName : "no macro defined", animationIndex+1);

        ImGui::PushFont(appState.fonts.large);
        ImGui::TextWrapped("Key no. %u", globalAnimatable->getCurrentKeyIndex() + 1);
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    RvlCellAnim::AnimationKey* animKey = globalAnimatable->getCurrentKey();

    ImGui::SeparatorText((char*)ICON_FA_IMAGE " Arrangement");

    // Arrangement Input
    {
        static uint16_t oldArrangement { 0 };
        uint16_t newArrangement = globalAnimatable->getCurrentKey()->arrangementIndex + 1;

        ImGui::SetNextItemWidth(
            ImGui::CalcItemWidth() -
            (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x) * 2
        );
        if (ImGui::InputScalar(
            "##Arrangement No.",
            ImGuiDataType_U16,
            &newArrangement,
            nullptr, nullptr,
            "%u"
        )) {
            globalAnimatable->getCurrentKey()->arrangementIndex =
                std::min<uint16_t>(newArrangement - 1, globalAnimatable->cellanim->arrangements.size() - 1);

            appState.correctSelectedParts();
        }

        if (ImGui::IsItemActivated())
            oldArrangement = originalKey.arrangementIndex;

        if (ImGui::IsItemDeactivated() && !appState.getArrangementMode()) {
            changed = true;

            originalKey.arrangementIndex = oldArrangement;
            newKey.arrangementIndex =
                std::min<uint16_t>(newArrangement - 1, globalAnimatable->cellanim->arrangements.size() - 1);
        }

        // Start +- Buttons

        const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

        ImGui::PushButtonRepeat(true);

        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        if (
            ImGui::Button("-##Arrangement No._dec", buttonSize) &&
            globalAnimatable->getCurrentKey()->arrangementIndex > 0
        ) {
            if (!appState.getArrangementMode())
                newKey.arrangementIndex--;
            else
                globalAnimatable->getCurrentKey()->arrangementIndex--;

            appState.correctSelectedParts();
        }
        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        if (
            ImGui::Button("+##Arrangement No._inc", buttonSize) &&
            (globalAnimatable->getCurrentKey()->arrangementIndex + 1) < globalAnimatable->cellanim->arrangements.size()
        ) {
            if (!appState.getArrangementMode())
                newKey.arrangementIndex++;
            else
                globalAnimatable->getCurrentKey()->arrangementIndex++;

            appState.correctSelectedParts();
        }

        ImGui::PopButtonRepeat();

        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text("Arrangement No.");
    }

    {
        bool arrangementUnique = CellanimHelpers::getArrangementUnique(globalAnimatable->getCurrentKey()->arrangementIndex);
        ImGui::BeginDisabled(arrangementUnique);

        if (ImGui::Button("Make arrangement unique (duplicate)"))
            newKey.arrangementIndex =
                CellanimHelpers::DuplicateArrangement(animKey->arrangementIndex);

        ImGui::EndDisabled();

        if (
            arrangementUnique &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)
        )
            ImGui::OpenPopup("DuplicateAnywayPopup");

        if (ImGui::BeginPopup("DuplicateAnywayPopup")) {
            ImGui::Text("Would you like to duplicate this\narrangement anyway?");
            ImGui::Separator();

            if (ImGui::Selectable("Ok"))
                newKey.arrangementIndex =
                    CellanimHelpers::DuplicateArrangement(animKey->arrangementIndex);
            ImGui::Selectable("Nevermind");

            ImGui::EndPopup();
        }

        if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("This arrangement is only used once.\nYou can right-click to duplicate anyway.");
    }

    ImGui::SeparatorText((char*)ICON_FA_PAUSE " Hold");

    {
        static uint16_t oldHoldFrames { 0 };
        uint16_t holdFrames = animKey->holdFrames;

        if (ImGui::InputScalar("Hold Frames", ImGuiDataType_U16, &holdFrames, &uint16_one, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (holdFrames <= 1)
                holdFrames = 1;

            animKey->holdFrames = holdFrames;
        };

        if (ImGui::IsItemActivated())
            oldHoldFrames = originalKey.holdFrames;

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.holdFrames = oldHoldFrames;
            newKey.holdFrames = holdFrames;
        }
    }

    ImGui::SeparatorText((char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform");

    // Position XY
    {
        static int oldPosition[2] { 0, 0 };
        int positionValues[2] {
            animKey->transform.positionX,
            animKey->transform.positionY
        };

        if (ImGui::DragInt2(
            "Position XY",
            positionValues, 1.f,
            std::numeric_limits<int16_t>::min(),
            std::numeric_limits<int16_t>::max()
        )) {
            animKey->transform.positionX = positionValues[0];
            animKey->transform.positionY = positionValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldPosition[0] = originalKey.transform.positionX;
            oldPosition[1] = originalKey.transform.positionY;
        }

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.transform.positionX = oldPosition[0];
            originalKey.transform.positionY = oldPosition[1];

            newKey.transform.positionX = positionValues[0];
            newKey.transform.positionY = positionValues[1];
        }
    }

    // Scale XY
    {
        static float oldScale[2] { 0.f, 0.f };
        float scaleValues[2] { animKey->transform.scaleX, animKey->transform.scaleY };

        if (ImGui::DragFloat2("Scale XY", scaleValues, .01f)) {
            animKey->transform.scaleX = scaleValues[0];
            animKey->transform.scaleY = scaleValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldScale[0] = originalKey.transform.scaleX;
            oldScale[1] = originalKey.transform.scaleY;
        }

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.transform.scaleX = oldScale[0];
            originalKey.transform.scaleY = oldScale[1];

            newKey.transform.scaleX = scaleValues[0];
            newKey.transform.scaleY = scaleValues[1];
        }
    }

    // Angle Z slider
    {
        static float oldAngle { 0.f };
        float newAngle = animKey->transform.angle;

        if (ImGui::SliderFloat("Angle Z", &newAngle, -360.f, 360.f, "%.1f deg"))
            animKey->transform.angle = newAngle;

        if (ImGui::IsItemActivated())
            oldAngle = originalKey.transform.angle;

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.transform.angle = oldAngle;
            newKey.transform.angle = newAngle;
        }
    }

    ImGui::SeparatorText((char*)ICON_FA_IMAGE " Rendering");

    // Opacity slider
    {
        static const uint8_t min { 0 };
        static const uint8_t max { 0xFF };

        static uint8_t oldOpacity { 0 };
        uint8_t newOpacity = animKey->opacity;
        if (ImGui::SliderScalar("Opacity", ImGuiDataType_U8, &newOpacity, &min, &max, "%u"))
            animKey->opacity = newOpacity;

        if (ImGui::IsItemActivated())
            oldOpacity = originalKey.opacity;

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.opacity = oldOpacity;
            newKey.opacity = newOpacity;
        }
    }

    if (newKey != originalKey) {
        *globalAnimatable->getCurrentKey() = originalKey;

        SessionManager::getInstance().getCurrentSession()->executeCommand(
        std::make_shared<CommandModifyAnimationKey>(
            sessionManager.getCurrentSession()->currentCellanim,
            appState.globalAnimatable->getCurrentAnimationIndex(),
            appState.globalAnimatable->getCurrentKeyIndex(),
            newKey
        ));
    }
}
