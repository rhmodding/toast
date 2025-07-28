#include "BezierUtil.hpp"

#include <cstdint>

#include <imgui_internal.h>

#include <cstdio>

#include <cmath>

#include <algorithm>

#include <array>

ImVec2 BezierUtil::Approx(const float x, const float P[4], unsigned maxIterations) {
    float t = x; // Initial guess for t

    for (unsigned i = 0; i < maxIterations; ++i) {
        float u = 1.f - t;

        float tt = t * t;
        float uu = u * u;

        float bezX = 3.f * uu * t * P[0] + 3.f * u * tt * P[2] + tt * t;

        float dBezX = 3.f * uu * P[0]
                    + 6.f * u * t * (P[2] - P[0])
                    + 3.f * tt * (1.f - P[2]);

        if (std::fabs(dBezX) < 1e-5f)
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

static void EvalStepsFixed(const float P[4], ImVec2* results, unsigned stepCount) {
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

bool BezierUtil::ImWidget(const char* label, float P[4], bool handlesEnabled) {
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
    std::snprintf(buf, sizeof(buf), "%s##bInteract", label);

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
    EvalStepsFixed(P, results.data(), LINE_SEGMENTS);

    // Drawing & handle drag behaviour
    {
        bool hoveredHandle[2] { false };
        static int handleBeingDragged { -1 };

        // Handle drag behaviour
        if (handlesEnabled) {
            for (int i = 0; i < 2; i++) {
                ImVec2 pos {
                    (P[i * 2 + 0]) * bb.GetWidth() + bb.Min.x,
                    (1.f - P[i * 2 + 1]) * bb.GetHeight() + bb.Min.y
                };

                hoveredHandle[i] = ImGui::IsMouseHoveringRect(
                    { pos.x - GRAB_RADIUS, pos.y - GRAB_RADIUS },
                    { pos.x + GRAB_RADIUS, pos.y + GRAB_RADIUS }
                );

                if (hoveredHandle[i] || handleBeingDragged == i)
                    ImGui::SetTooltip("(%4.3f, %4.3f)", P[i * 2 + 0], P[i * 2 + 1]);

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
