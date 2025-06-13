#ifndef POPUP_MINTERPOLATEKEYS_HPP
#define POPUP_MINTERPOLATEKEYS_HPP

#include <imgui.h>

#include <cmath>

#include <array>

#include "util/BezierUtil.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "command/CommandModifyAnimation.hpp"

#include "manager/PlayerManager.hpp"

#include "SelectionState.hpp"

#include "Macro.hpp"

static void _ApplyInterpolation(
    const std::array<float, 4>& curve,
    int interval, // Spacing between frames.
    const CellAnim::AnimationKey& backKey, const CellAnim::AnimationKey& frontKey,
    const CellAnim::Animation& animation, unsigned animationIndex
) {
    auto& arrangements = SessionManager::getInstance().getCurrentSession()
        ->getCurrentCellAnim().object->getArrangements();

    int addKeysCount = (backKey.holdFrames / interval) - 1;
    if (addKeysCount <= 0)
        return;

    std::vector<CellAnim::AnimationKey> addKeys(addKeysCount);

    // unsigned totalFrames = interval;
    for (unsigned i = 0; i < addKeysCount; i++) {
        CellAnim::AnimationKey& newKey = addKeys[i];
        newKey = backKey;

        float t = BezierUtil::ApproxY((((i+1) * interval)) / (float)backKey.holdFrames, curve.data());

        newKey.holdFrames = interval;
        // totalFrames += interval;

        newKey.transform = newKey.transform.lerp(frontKey.transform, t);
        newKey.opacity = static_cast<uint8_t>(std::clamp<int>(
            LERP_INTS(backKey.opacity, frontKey.opacity, t),
            0x00, 0xFF
        ));

        newKey.foreColor = newKey.foreColor.lerp(frontKey.foreColor, t);
        newKey.backColor = newKey.backColor.lerp(frontKey.backColor, t);

        // Create new interpolated arrangement if not equal
        if (backKey.arrangementIndex != frontKey.arrangementIndex) {
            CellAnim::Arrangement newArrangement = arrangements.at(backKey.arrangementIndex);

            for (unsigned j = 0; j < newArrangement.parts.size(); j++) {
                const CellAnim::Arrangement& endArrangement = arrangements.at(frontKey.arrangementIndex);

                auto& part = newArrangement.parts.at(j);

                unsigned jClamp = std::min<unsigned>(j, endArrangement.parts.size() - 1);
                int nameMatcher = SelectionState::getMatchingNamePartIndex(part, endArrangement, j);

                const auto* endPart = &endArrangement.parts.at(nameMatcher >= 0 ? nameMatcher : jClamp);

                if (nameMatcher < 0 && (
                    endPart->regionPos != part.regionPos ||
                    endPart->regionSize != part.regionSize
                )) {
                    int mrpi = SelectionState::getMatchingRegionPartIndex(part, endArrangement, j);

                    if (mrpi >= 0)
                        endPart = &endArrangement.parts.at(mrpi);
                }
                part.transform = part.transform.lerp(endPart->transform, t);

                // Flip over if curve Y is over half-point
                if (t > .5f) {
                    part.flipX = endPart->flipX;
                    part.flipY = endPart->flipY;

                    part.regionPos = endPart->regionPos;
                    part.regionSize = endPart->regionSize;
                }

                part.opacity = std::clamp<int>(
                    LERP_INTS(part.opacity, endPart->opacity, t),
                    0x00, 0xFF
                );

                part.foreColor = part.foreColor.lerp(endPart->foreColor, t);
                part.backColor = part.backColor.lerp(endPart->backColor, t);
            }

            newKey.arrangementIndex = arrangements.size();
            arrangements.push_back(newArrangement);
        }
    }

    SessionManager& sessionManager = SessionManager::getInstance();

    unsigned currentKeyIndex = PlayerManager::getInstance().getKeyIndex();

    CellAnim::Animation newAnim = animation;
    newAnim.keys.insert(newAnim.keys.begin() + currentKeyIndex + 1, addKeys.begin(), addKeys.end());

    newAnim.keys.at(currentKeyIndex).holdFrames = interval;

    sessionManager.getCurrentSession()->addCommand(
    std::make_shared<CommandModifyAnimation>(
        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
        animationIndex,
        newAnim
    ));
}

static void Popup_MInterpolateKeys() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

    static bool lateOpen { false };
    const bool active = ImGui::BeginPopup("MInterpolateKeys");

    bool condition = SessionManager::getInstance().getCurrentSessionIndex() >= 0;

    PlayerManager& playerManager = PlayerManager::getInstance();

    static CellAnim::Animation animationBackup { CellAnim::Animation {} };

    CellAnim::Animation* currentAnimation { nullptr };
    unsigned animationIndex;

    CellAnim::AnimationKey* currentKey { nullptr };
    int currentKeyIndex = -1;

    CellAnim::AnimationKey* nextKey { nullptr };

    if (condition) {
        currentAnimation = &playerManager.getAnimation();
        animationIndex = playerManager.getAnimationIndex();

        currentKey = &playerManager.getKey();
        currentKeyIndex = static_cast<int>(playerManager.getKeyIndex());

        bool nextKeyExists = currentKeyIndex + 1 < static_cast<int>(currentAnimation->keys.size());

        if (nextKeyExists)
            nextKey = &playerManager.getAnimation().keys.at(currentKeyIndex + 1);

        condition = nextKeyExists;
    }

    if (!active && lateOpen && condition) {
        *currentAnimation = animationBackup;
        playerManager.correctState();

        lateOpen = false;
    }

    if (active && condition) {
        if (!lateOpen) {
            animationBackup = *currentAnimation;
        }

        lateOpen = true;

        ImGui::SeparatorText("Tweening options");

        static std::array<float, 4> v { 0.f, 0.f, 1.f, 1.f };
        static int interval = 1;

        {
            static bool customEasing { false };

            static const char* easeNames[] {
                "Linear",
                "Ease In (Sine)",
                "Ease Out (Sine)",
                "Ease In-Out (Sine)",
                "Ease In (Quad)",
                "Ease Out (Quad)",
                "Ease In-Out (Quad)",
                "Ease In (Cubic)",
                "Ease Out (Cubic)",
                "Ease In-Out (Cubic)",
                "Ease In (Quart)",
                "Ease Out (Quart)",
                "Ease In-Out (Quart)",
                "Ease In (Quint)",
                "Ease Out (Quint)",
                "Ease In-Out (Quint)",
                "Ease In (Expo)",
                "Ease Out (Expo)",
                "Ease In-Out (Expo)",
                "Ease In (Circ)",
                "Ease Out (Circ)",
                "Ease In-Out (Circ)",
                "Ease In (Back)",
                "Ease Out (Back)",
                "Ease In-Out (Back)",
                "Custom"
            };
            static const std::array<float, 4> easePresets[] {
                { 0.000f, 0.000f, 1.000f, 1.000f }, // Linear
                { 0.470f, 0.000f, 0.745f, 0.715f }, // Ease In (Sine)
                { 0.390f, 0.575f, 0.565f, 1.000f }, // Ease Out (Sine)
                { 0.445f, 0.050f, 0.550f, 0.950f }, // Ease In-Out (Sine)
                { 0.550f, 0.085f, 0.680f, 0.530f }, // Ease In (Quad)
                { 0.250f, 0.460f, 0.450f, 0.940f }, // Ease Out (Quad)
                { 0.455f, 0.030f, 0.515f, 0.955f }, // Ease In-Out (Quad)
                { 0.550f, 0.055f, 0.675f, 0.190f }, // Ease In (Cubic)
                { 0.215f, 0.610f, 0.355f, 1.000f }, // Ease Out (Cubic)
                { 0.645f, 0.045f, 0.355f, 1.000f }, // Ease In-Out (Cubic)
                { 0.895f, 0.030f, 0.685f, 0.220f }, // Ease In (Quart)
                { 0.165f, 0.840f, 0.440f, 1.000f }, // Ease Out (Quart)
                { 0.770f, 0.000f, 0.175f, 1.000f }, // Ease In-Out (Quart)
                { 0.755f, 0.050f, 0.855f, 0.060f }, // Ease In (Quint)
                { 0.230f, 1.000f, 0.320f, 1.000f }, // Ease Out (Quint)
                { 0.860f, 0.000f, 0.070f, 1.000f }, // Ease In-Out (Quint)
                { 0.950f, 0.050f, 0.795f, 0.035f }, // Ease In (Expo)
                { 0.190f, 1.000f, 0.220f, 1.000f }, // Ease Out (Expo)
                { 1.000f, 0.000f, 0.000f, 1.000f }, // Ease In-Out (Expo)
                { 0.600f, 0.040f, 0.980f, 0.335f }, // Ease In (Circ)
                { 0.075f, 0.820f, 0.165f, 1.000f }, // Ease Out (Circ)
                { 0.785f, 0.135f, 0.150f, 0.860f }, // Ease In-Out (Circ)
                { 0.600f, -0.28f, 0.735f, 0.045f }, // Ease In (Back)
                { 0.175f, 0.885f, 0.320f, 1.275f }, // Ease Out (Back)
                { 0.680f, -0.55f, 0.265f, 1.550f }, // Ease In-Out (Back)

                { 0.445f, 0.050f, 0.550f, 0.950f }, // Custom (not applied)
            };

            int selectedEaseIndex { 0 };
            if (customEasing)
                selectedEaseIndex = ARRAY_LENGTH(easePresets) - 1;
            else {
                const std::array<float, 4>* easeSearch =
                    std::find(easePresets, easePresets + ARRAY_LENGTH(easePresets) - 1, v);
                selectedEaseIndex = easeSearch - easePresets;
            }

            BezierUtil::ImWidget("CurveView", v.data(), customEasing);

            ImGui::SameLine();

            ImGui::BeginGroup();

            if (ImGui::SliderFloat4("Curve", v.data(), 0, 1))
                customEasing = true;
            ImGui::Dummy({ 0.f, 6.f });

            ImGui::Separator();

            if (ImGui::Combo("Easing Preset", &selectedEaseIndex, easeNames, ARRAY_LENGTH(easeNames))) {
                customEasing = selectedEaseIndex == ARRAY_LENGTH(easePresets) - 1;

                if (!customEasing)
                    v = easePresets[selectedEaseIndex];
            }

            ImGui::Dummy({ 0.f, 6.f });

            ImGui::Separator();

            ImGui::Dummy({ 0.f, 2.f });

            ImGui::InputInt("Interval", &interval);

            ImGui::EndGroup();

            float t = fmodf((float)ImGui::GetTime(), 1.f);

            ImGui::ProgressBar(BezierUtil::ApproxY(t, v.data()));
        }

        ImGui::Separator();

        if (ImGui::Button("Apply")) {
            _ApplyInterpolation(
                v, interval,
                *currentKey, *nextKey,
                *currentAnimation, animationIndex
            );

            // Refresh pointers since parent vector could have reallocated
            currentAnimation = &playerManager.getAnimation();
            currentKey = &playerManager.getKey();
            nextKey = currentKey + 1;

            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            lateOpen = false;
        }
    }

    if (active)
        ImGui::EndPopup();

    ImGui::PopStyleVar();
}

#endif // POPUP_MINTERPOLATEKEYS_HPP
