#include "../WindowInspector.hpp"

#include "../../SessionManager.hpp"

#include "../../ThemeManager.hpp"

#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../font/FontAwesome.h"

#include "../../AppState.hpp"

void WindowInspector::Level_Key() {
    AppState& appState = AppState::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    this->drawPreview();

    ImGui::SameLine();

    const auto& arrangements = sessionManager.getCurrentSession()
        ->getCurrentCellanim().object->arrangements;

    CellAnim::AnimationKey newKey = playerManager.getKey();
    CellAnim::AnimationKey originalKey = playerManager.getKey();

    unsigned animationIndex = playerManager.getAnimationIndex();
    const char* animationName = playerManager.getAnimation().name.c_str();
    if (animationName[0] == '\0')
        animationName = "(no name set)";

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Anim \"%s\" (no. %u)", animationName, animationIndex+1);

        ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
        ImGui::TextWrapped("Key no. %u", playerManager.getKeyIndex() + 1);
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    CellAnim::AnimationKey& animKey = playerManager.getKey();

    ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Arrangement");

    // Arrangement Input
    {
        static int oldArrangement { 0 };
        int newArrangement = playerManager.getArrangementIndex() + 1;

        ImGui::SetNextItemWidth(
            ImGui::CalcItemWidth() -
            (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x) * 2
        );

        if (ImGui::InputInt("##Arrangement No.", &newArrangement)) {
            if (newArrangement < 1) newArrangement = 1;

            playerManager.getKey().arrangementIndex = std::min<unsigned>(
                newArrangement - 1, arrangements.size() - 1
            );
            playerManager.correctState();
        }

        if (ImGui::IsItemActivated())
            oldArrangement = originalKey.arrangementIndex;

        if (ImGui::IsItemDeactivated() && !appState.getArrangementMode()) {
            originalKey.arrangementIndex = oldArrangement;
            newKey.arrangementIndex = std::min<unsigned>(
                newArrangement - 1, arrangements.size() - 1
            );
        }

        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextUnformatted("Arrangement No.");
    }

    this->duplicateArrangementButton(newKey, originalKey);

    ImGui::SeparatorText((const char*)ICON_FA_PAUSE " Hold");

    {
        static unsigned oldHoldFrames { 0 };
        int holdFrames = animKey.holdFrames;

        if (ImGui::InputInt("Hold Frames", &holdFrames)) {
            animKey.holdFrames = std::clamp<unsigned>(
                holdFrames,
                CellAnim::AnimationKey::MIN_HOLD_FRAMES,
                CellAnim::AnimationKey::MAX_HOLD_FRAMES
            );
        };

        if (ImGui::IsItemActivated())
            oldHoldFrames = originalKey.holdFrames;

        if (ImGui::IsItemDeactivated()) {
            originalKey.holdFrames = std::clamp<unsigned>(
                oldHoldFrames,
                CellAnim::AnimationKey::MIN_HOLD_FRAMES,
                CellAnim::AnimationKey::MAX_HOLD_FRAMES
            );;
            newKey.holdFrames = std::clamp<unsigned>(
                holdFrames,
                CellAnim::AnimationKey::MIN_HOLD_FRAMES,
                CellAnim::AnimationKey::MAX_HOLD_FRAMES
            );;
        }
    }

    ImGui::SeparatorText((const char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform");

    // Position XY
    {
        static int oldPosition[2] { 0, 0 };
        int positionValues[2] {
            animKey.transform.positionX,
            animKey.transform.positionY
        };

        if (ImGui::DragInt2(
            "Position XY",
            positionValues, 1.f,
            CellAnim::TransformValues::MIN_POSITION,
            CellAnim::TransformValues::MAX_POSITION
        )) {
            animKey.transform.positionX = positionValues[0];
            animKey.transform.positionY = positionValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldPosition[0] = originalKey.transform.positionX;
            oldPosition[1] = originalKey.transform.positionY;
        }

        if (ImGui::IsItemDeactivated()) {
            originalKey.transform.positionX = oldPosition[0];
            originalKey.transform.positionY = oldPosition[1];

            newKey.transform.positionX = positionValues[0];
            newKey.transform.positionY = positionValues[1];
        }
    }

    // Scale XY
    {
        static float oldScale[2] { 0.f, 0.f };
        float scaleValues[2] { animKey.transform.scaleX, animKey.transform.scaleY };

        if (ImGui::DragFloat2("Scale XY", scaleValues, .01f)) {
            animKey.transform.scaleX = scaleValues[0];
            animKey.transform.scaleY = scaleValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldScale[0] = originalKey.transform.scaleX;
            oldScale[1] = originalKey.transform.scaleY;
        }

        if (ImGui::IsItemDeactivated()) {
            originalKey.transform.scaleX = oldScale[0];
            originalKey.transform.scaleY = oldScale[1];

            newKey.transform.scaleX = scaleValues[0];
            newKey.transform.scaleY = scaleValues[1];
        }
    }

    // Angle Z slider
    {
        static float oldAngle { 0.f };
        float newAngle = animKey.transform.angle;

        if (ImGui::SliderFloat("Angle Z", &newAngle, -360.f, 360.f, "%.1f deg"))
            animKey.transform.angle = newAngle;

        if (ImGui::IsItemActivated())
            oldAngle = originalKey.transform.angle;

        if (ImGui::IsItemDeactivated()) {
            originalKey.transform.angle = oldAngle;
            newKey.transform.angle = newAngle;
        }
    }

    ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Rendering");

    // Opacity slider
    {
        static const uint8_t min { 0 };
        static const uint8_t max { 0xFF };

        static uint8_t oldOpacity { 0 };
        uint8_t newOpacity = animKey.opacity;
        if (ImGui::SliderScalar("Opacity", ImGuiDataType_U8, &newOpacity, &min, &max, "%u"))
            animKey.opacity = newOpacity;

        if (ImGui::IsItemActivated())
            oldOpacity = originalKey.opacity;

        if (ImGui::IsItemDeactivated()) {
            originalKey.opacity = oldOpacity;
            newKey.opacity = newOpacity;
        }
    }

    if (newKey != originalKey) {
        playerManager.getKey() = originalKey;

        sessionManager.getCurrentSession()->addCommand(
        std::make_shared<CommandModifyAnimationKey>(
            sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
            playerManager.getAnimationIndex(),
            playerManager.getKeyIndex(),
            newKey
        ));
    }
}
