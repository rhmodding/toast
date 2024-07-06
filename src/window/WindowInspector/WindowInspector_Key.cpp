#include "../WindowInspector.hpp"

#include "../../font/FontAwesome.h"

#include "../../SessionManager.hpp"

#include "../../command/CommandInsertArrangement.hpp"
#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../AppState.hpp"
#include "../../anim/Animatable.hpp"

const uint16_t uint16_one = 1;

bool getArrangementUnique(uint16_t index) {
    uint8_t timesUsed{ 0 };

    for (const auto& animation : SessionManager::getInstance().getCurrentSession()->getCellanimObject()->animations) {
        for (const auto& key : animation.keys) {
            if (key.arrangementIndex == index)
                timesUsed++;

            if (timesUsed > 1) {
                return false;
            }
        }
    }

    return true;
}

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

    ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

        ImGui::Text("Anim \"%s\" (no. %u)", animationName ? animationName : "no macro defined", animationIndex+1);

        ImGui::PushFont(appState.fontLarge);
        ImGui::TextWrapped("Key no. %u", globalAnimatable->getCurrentKeyIndex()+1);
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    RvlCellAnim::AnimationKey* animKey = globalAnimatable->getCurrentKey();

    ImGui::SeparatorText((char*)ICON_FA_IMAGE " Arrangement");

    // Arrangement Input
    {
        static uint16_t oldArrangement{ 0 };
        uint16_t newArrangement = globalAnimatable->getCurrentKey()->arrangementIndex + 1;

        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x) * 2);
        if (ImGui::InputScalar(
            "##Arrangement No.",
            ImGuiDataType_U16,
            &newArrangement,
            nullptr, nullptr,
            "%u"
        )) {
            globalAnimatable->getCurrentKey()->arrangementIndex =
                std::clamp<uint16_t>(newArrangement - 1, 0, globalAnimatable->cellanim->arrangements.size());

            appState.selectedPart = std::clamp<int32_t>(
                appState.selectedPart,
                -1,
                globalAnimatable->getCurrentArrangement()->parts.size() - 1
            );
        }

        if (ImGui::IsItemActivated())
            oldArrangement = originalKey.arrangementIndex;

        if (ImGui::IsItemDeactivated() && !appState.getArrangementMode()) {
            changed = true;

            originalKey.arrangementIndex = oldArrangement;
            newKey.arrangementIndex =
                std::clamp<uint16_t>(newArrangement - 1, 0, globalAnimatable->cellanim->arrangements.size());
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

            appState.selectedPart = std::clamp<int32_t>(
                appState.selectedPart,
                -1,
                globalAnimatable->getCurrentArrangement()->parts.size() - 1
            );
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

            appState.selectedPart = std::clamp<int32_t>(
                appState.selectedPart,
                -1,
                globalAnimatable->getCurrentArrangement()->parts.size() - 1
            );
        }

        ImGui::PopButtonRepeat();

        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text("Arrangement No.");
    }

    bool arrangementUnique = getArrangementUnique(globalAnimatable->getCurrentKey()->arrangementIndex);
    ImGui::BeginDisabled(arrangementUnique);

    if (ImGui::Button("Make arrangement unique (duplicate)")) {
        SessionManager::getInstance().getCurrentSession()->executeCommand(
        std::make_shared<CommandInsertArrangement>(
        CommandInsertArrangement(
            sessionManager.getCurrentSession()->currentCellanim,
            sessionManager.getCurrentSession()->getCellanimObject()->arrangements.size(),
            sessionManager.getCurrentSession()->getCellanimObject()->arrangements.at(animKey->arrangementIndex)
        )));

        newKey.arrangementIndex = 
            sessionManager.getCurrentSession()->getCellanimObject()->arrangements.size() - 1;
    }

    ImGui::EndDisabled();

    if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("This arrangement is only used once.");

    ImGui::SeparatorText((char*)ICON_FA_PAUSE " Hold");

    {
        static uint16_t oldHoldFrames{ 0 };
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
        static int oldPosition[2]{ 0, 0 };
        int positionValues[2] = {
            animKey->positionX,
            animKey->positionY
        };
        if (ImGui::DragInt2("Position XY", positionValues, 1.f, -UINT8_MAX, UINT8_MAX)) {
            animKey->positionX = positionValues[0];
            animKey->positionY = positionValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldPosition[0] = originalKey.positionX;
            oldPosition[1] = originalKey.positionY;
        }

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.positionX = oldPosition[0];
            originalKey.positionY = oldPosition[1];

            newKey.positionX = positionValues[0];
            newKey.positionY = positionValues[1];
        }
    }

    // Scale XY
    {
        static float oldScale[2]{ 0.f, 0.f };
        float scaleValues[2] = { animKey->scaleX, animKey->scaleY };
        if (ImGui::DragFloat2("Scale XY", scaleValues, 0.01f)) {
            animKey->scaleX = scaleValues[0];
            animKey->scaleY = scaleValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldScale[0] = originalKey.scaleX;
            oldScale[1] = originalKey.scaleY;
        }

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.scaleX = oldScale[0];
            originalKey.scaleY = oldScale[1];

            newKey.scaleX = scaleValues[0];
            newKey.scaleY = scaleValues[1];
        }
    }

    // Angle Z slider
    {
        static float oldAngle{ 0.f };
        float newAngle = animKey->angle;
        if (ImGui::SliderFloat("Angle Z", &newAngle, -360.f, 360.f, "%.1f deg"))
            animKey->angle = newAngle;

        if (ImGui::IsItemActivated())
            oldAngle = originalKey.angle;

        if (ImGui::IsItemDeactivated()) {
            changed = true;

            originalKey.angle = oldAngle;
            newKey.angle = newAngle;
        }
    }

    ImGui::SeparatorText((char*)ICON_FA_IMAGE " Rendering");

    // Opacity slider
    {
        static const uint8_t min{ 0 };
        static const uint8_t max{ 0xFF };

        static uint8_t oldOpacity{ 0 };
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



    /*

    if (ConfigManager::getInstance().config.showUnknownValues) {
        ImGui::SeparatorText((char*)ICON_FA_CIRCLE_QUESTION " Unknown value (byteswapped)..");

        if (ImGui::CollapsingHeader("..as Uint32", ImGuiTreeNodeFlags_None)) {
            changed |= ImGui::InputScalar(" A", ImGuiDataType_U32, &animKey->unknown.u32, &uint32_one, nullptr, "%u");
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("..as Uint16", ImGuiTreeNodeFlags_None)) {
            changed |= ImGui::InputScalar(" A", ImGuiDataType_U16, &animKey->unknown.u16[0], &uint16_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" B", ImGuiDataType_U16, &animKey->unknown.u16[1], &uint16_one, nullptr, "%u");
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("..as Uint8 (byte)", ImGuiTreeNodeFlags_None)) {
            changed |= ImGui::InputScalar(" A", ImGuiDataType_U8, &animKey->unknown.u8[0], &uint8_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" B", ImGuiDataType_U8, &animKey->unknown.u8[1], &uint8_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" C", ImGuiDataType_U8, &animKey->unknown.u8[2], &uint8_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" D", ImGuiDataType_U8, &animKey->unknown.u8[3], &uint8_one, nullptr, "%u");
        }
    }

    */

    if (newKey != originalKey) {
        *globalAnimatable->getCurrentKey() = originalKey;

        SessionManager::getInstance().getCurrentSession()->executeCommand(
        std::make_shared<CommandModifyAnimationKey>(
        CommandModifyAnimationKey(
            sessionManager.getCurrentSession()->currentCellanim,
            appState.globalAnimatable->getCurrentAnimationIndex(),
            appState.globalAnimatable->getCurrentKeyIndex(),
            newKey
        )));
    }
}