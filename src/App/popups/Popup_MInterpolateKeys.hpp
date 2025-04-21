#ifndef POPUP_MINTERPOLATEKEYS_HPP
#define POPUP_MINTERPOLATEKEYS_HPP

#include <imgui.h>

#include <cmath>

#include <array>

#include "../../cellanim/CellAnim.hpp"

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyAnimation.hpp"

#include "../../PlayerManager.hpp"

#include "../../SelectionState.hpp"

#include "../../common.hpp"

namespace _Bezier {

// results must be ImVec2[stepCount + 1]
static void EvalBezierSteps(const float P[4], ImVec2* results, unsigned stepCount) {
    if (stepCount == 0)
        return;

    const float step = 1.f / stepCount;
    float t = step;

    for (unsigned i = 1; i <= stepCount; ++i, t += step) {
        float u = 1.f - t;
        float t2 = t * t;
        float t3 = t2 * t;
        float u2 = u * u;

        float b0 = u2 * u;
        float b1 = 3.f * u2 * t;
        float b2 = 3.f * u * t2;
        float b3 = t3;

        results[i] = ImVec2(
            b0 * 0.f + b1 * P[0] + b2 * P[2] + b3 * 1.f,
            b0 * 0.f + b1 * P[1] + b2 * P[3] + b3 * 1.f
        );
    }
}

// Newton-Raphson
// Results in a tiny X error (usually not more than about +- 0.0000006 on 10 iterations).
static ImVec2 BezierValue(const float x, const float P[4], unsigned maxIterations = 10) {
    float t = x; // Initial guess for t
    
    for (unsigned i = 0; i < maxIterations; ++i) {
        float u = 1.f - t;

        float tt = t * t;
        float uu = u * u;

        float bezX = 3.f * uu * t * P[0] + 3.f * u * tt * P[2] + tt * t;

        float dBezX = 3.f * uu * P[0]
                    + 6.f * u * t * (P[2] - P[0])
                    + 3.f * tt * (1.f - P[2]);

        if (fabsf(dBezX) < 1e-5f)
            break;

        float delta = bezX - x;
        t -= delta / dBezX;

        t = std::clamp<float>(t, 0.f, 1.f);
    }

    float u = 1.f - t;
    float tt = t * t;
    float uu = u * u;

    float b1 = 3.f * uu * t;
    float b2 = 3.f * u * tt;
    float t3 = tt * t;

    return ImVec2(
        b1 * P[0] + b2 * P[2] + t3,
        b1 * P[1] + b2 * P[3] + t3
    );
}

static float BezierValueY(const float x, const float P[4], unsigned maxIterations = 10) {
    return BezierValue(x, P, maxIterations).y;
}

static bool Widget(const char* label, float P[4], bool handlesEnabled = true) {
    constexpr int LINE_SEGMENTS = 32; // Line segments in rendered curve
    constexpr int CURVE_WIDTH = 4; // Curve line width

    constexpr int LINE_WIDTH = 1; // Handles: connecting line width
    constexpr int GRAB_RADIUS = 6; // Handles: circle radius
    constexpr int GRAB_BORDER = 2; // Handles: circle border width

    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiIO& IO = ImGui::GetIO();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return false;

    bool changed { false };

    const float avail = ImGui::GetContentRegionAvail().x;

    const float padX = 32.f;
    const float padY = 144.f;

    ImRect outerBb = {
        window->DC.CursorPos,
        {
            window->DC.CursorPos.x + ImMin(avail + 1.f, 256.f + padX),
            window->DC.CursorPos.y + ImMin(avail + 1.f, 192.f + padY)
        }
    };

    ImRect bb = outerBb;
    bb.Expand({ -(padX / 2.f), -(padY / 2.f) });

    char buf[128];
    snprintf(buf, sizeof(buf), "%s##bInteract", label);

    if (outerBb.GetArea() != 0.f)
        ImGui::InvisibleButton(buf, outerBb.GetSize(),
            ImGuiButtonFlags_MouseButtonLeft
        );

    //const bool interactionHovered     = ImGui::IsItemHovered();
    const bool interactionActive      = ImGui::IsItemActive();      // Held
    const bool interactionDeactivated = ImGui::IsItemDeactivated(); // Un-held

    // Dragging
    bool draggingLeft = interactionActive &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.f);

    ImGui::RenderFrame( // Outer frame
        outerBb.Min, outerBb.Max,
        ImGui::GetColorU32(ImGuiCol_FrameBg, .75f), true,
        style.FrameRounding
    );
    ImGui::RenderFrame( // Inner frame
        bb.Min, bb.Max,
        ImGui::GetColorU32(ImGuiCol_FrameBg, 1.f), true,
        style.FrameRounding
    );

    // Draw grid
    for (unsigned i = 0; i <= bb.GetWidth(); i += IM_ROUND(bb.GetWidth() / 4)) {
        drawList->AddLine(
            { bb.Min.x + i, outerBb.Min.y },
            { bb.Min.x + i, outerBb.Max.y },
            ImGui::GetColorU32(ImGuiCol_TextDisabled)
        );
    }
    for (unsigned i = 0; i <= bb.GetHeight(); i += IM_ROUND(bb.GetHeight() / 4)) {
        drawList->AddLine(
            { outerBb.Min.x, bb.Min.y + i },
            { outerBb.Max.x, bb.Min.y + i },
            ImGui::GetColorU32(ImGuiCol_TextDisabled)
        );
    }

    // Evaluate curve
    std::array<ImVec2, LINE_SEGMENTS + 1> results;
    EvalBezierSteps(P, results.data(), LINE_SEGMENTS);

    // Drawing & handle drag behaviour
    {
        bool hoveredHandle[2] { false };
        static int handleBeingDragged { -1 };

        // Handle drag behaviour
        if (handlesEnabled) {
            for (int i = 0; i < 2; i++) {
                ImVec2 pos {
                    (P[i*2+0]) * bb.GetWidth() + bb.Min.x,
                    (1 - P[i*2+1]) * bb.GetHeight() + bb.Min.y
                };

                hoveredHandle[i] = ImGui::IsMouseHoveringRect(
                    { pos.x - GRAB_RADIUS, pos.y - GRAB_RADIUS },
                    { pos.x + GRAB_RADIUS, pos.y + GRAB_RADIUS }
                );

                if (hoveredHandle[i] || handleBeingDragged == i)
                    ImGui::SetTooltip("(%4.3f, %4.3f)", P[i*2+0], P[i*2+1]);

                if (
                    draggingLeft &&
                    handleBeingDragged < 0 &&
                    hoveredHandle[i] != 0
                )
                    handleBeingDragged = i;

                if (handleBeingDragged == i) {
                    P[i*2+0] += IO.MouseDelta.x / bb.GetWidth();
                    P[i*2+1] -= IO.MouseDelta.y / bb.GetHeight();

                    P[i*2+0] = std::clamp<float>(P[i*2+0], 0.f, 1.f);

                    changed = true;
                }
            }

            if (interactionDeactivated)
                handleBeingDragged = -1;
        }

        drawList->PushClipRect(outerBb.Min, outerBb.Max);

        // Draw curve
        {
            ImVec2 points[LINE_SEGMENTS + 1];
            for (unsigned i = 0; i < LINE_SEGMENTS + 1; i++) {
                points[i] = {
                    results[i+0].x * bb.GetWidth() + bb.Min.x,
                    (1 - results[i+0].y) * bb.GetHeight() + bb.Min.y
                };
            }

            const ImColor color = ImGui::GetStyle().Colors[ImGuiCol_PlotLines];
            drawList->AddPolyline(
                points, LINE_SEGMENTS + 1,
                color,
                ImDrawFlags_RoundCornersAll,
                CURVE_WIDTH
            );
        }

        // Draw handles
        if (handlesEnabled) {
            float lumaA =
                (hoveredHandle[0] || handleBeingDragged == 0) ?
                .5f : 1.f;
            float lumaB =
                (hoveredHandle[1] || handleBeingDragged == 1) ?
                .5f : 1.f;

            const uint32_t
                handleColA (ImGui::ColorConvertFloat4ToU32({
                    1.00f, 0.00f, 0.75f, lumaA
                })),
                handleColB (ImGui::ColorConvertFloat4ToU32({
                    0.00f, 0.75f, 1.00f, lumaB
                }));

            ImVec2 p1 {
                P[0] * bb.GetWidth() + bb.Min.x,
                (1.f - P[1]) * bb.GetHeight() + bb.Min.y
            };
            ImVec2 p2 {
                P[2] * bb.GetWidth() + bb.Min.x,
                (1.f - P[3]) * bb.GetHeight() + bb.Min.y
            };

            drawList->AddLine(bb.GetBL(), p1, IM_COL32(0,0,0,0xFF), LINE_WIDTH);
            drawList->AddLine(bb.GetTR(), p2, IM_COL32(0,0,0,0xFF), LINE_WIDTH);

            drawList->AddCircleFilled(p1, GRAB_RADIUS, IM_COL32(0,0,0,0xFF));
            drawList->AddCircleFilled(p1, GRAB_RADIUS-GRAB_BORDER, handleColA);
            drawList->AddCircleFilled(p2, GRAB_RADIUS, IM_COL32(0,0,0,0xFF));
            drawList->AddCircleFilled(p2, GRAB_RADIUS-GRAB_BORDER, handleColB);
        }

        drawList->PopClipRect();
    }

    return changed;
}

} // namespace Bezier

static void _ApplyInterpolation(
    const std::array<float, 4>& curve,
    int interval, // Spacing between frames.
    const CellAnim::AnimationKey& backKey, const CellAnim::AnimationKey& frontKey,
    const CellAnim::Animation& animation, unsigned animationIndex
) {
    AppState& appState = AppState::getInstance();

    auto& arrangements = SessionManager::getInstance().getCurrentSession()
        ->getCurrentCellAnim().object->arrangements;

    int addKeysCount = (backKey.holdFrames / interval) - 1;
    if (addKeysCount <= 0)
        return;

    std::vector<CellAnim::AnimationKey> addKeys(addKeysCount);

    unsigned totalFrames = interval;
    for (unsigned i = 0; i < addKeysCount; i++) {
        CellAnim::AnimationKey& newKey = addKeys[i];
        newKey = backKey;

        float t = _Bezier::BezierValueY((((i+1) * interval)) / (float)backKey.holdFrames, curve.data());

        newKey.holdFrames = interval;
        totalFrames += interval;

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
                int nameMatcher = SelectionState::getMatchingNamePartIndex(part, endArrangement);

                const auto* endPart = &endArrangement.parts.at(nameMatcher >= 0 ? nameMatcher : jClamp);

                if (nameMatcher < 0 && (
                    endPart->regionX != part.regionX ||
                    endPart->regionY != part.regionY ||
                    endPart->regionW != part.regionW ||
                    endPart->regionH != part.regionH
                )) {
                    int mrpi = SelectionState::getMatchingRegionPartIndex(part, endArrangement);

                    if (mrpi >= 0)
                        endPart = &endArrangement.parts.at(mrpi);
                }
                part.transform = part.transform.lerp(endPart->transform, t);

                // Flip over if curve Y is over half-point
                if (t > .5f) {
                    part.flipX = endPart->flipX;
                    part.flipY = endPart->flipY;

                    part.regionX = endPart->regionX;
                    part.regionY = endPart->regionY;
                    part.regionW = endPart->regionW;
                    part.regionH = endPart->regionH;
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

            _Bezier::Widget("CurveView", v.data(), customEasing);

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

            ImGui::ProgressBar(_Bezier::BezierValueY(t, v.data()));
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
