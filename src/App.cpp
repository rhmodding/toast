#include "App.hpp"

#include <algorithm>
#include <array>

#include <cmath>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <chrono>

#include <iostream>

#include <string>
#include <sstream>

//#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <tinyfiledialogs.h>

#include "anim/RvlCellAnim.hpp"

#include "font/SegoeUI.h"
#include "font/FontAwesome.h"

#include "AppState.hpp"

#include "SessionManager.hpp"
#include "ConfigManager.hpp"
#include "PlayerManager.hpp"

#include "command/CommandSwitchCellanim.hpp"
#include "command/CommandModifyArrangement.hpp"
#include "command/CommandDeleteArrangement.hpp"
#include "command/CommandModifyArrangementPart.hpp"
#include "command/CommandDeleteArrangementPart.hpp"
#include "command/CommandModifyAnimationKey.hpp"
#include "command/CommandDeleteAnimationKey.hpp"

#include "anim/CellanimHelpers.hpp"

#include "task/AsyncTaskManager.hpp"
#include "task/AsyncTask_ExportSessionTask.hpp"
#include "task/AsyncTask_PushSessionTask.hpp"
#include "task/AsyncTask_OptimizeCellanim.hpp"

#include "MtCommandManager.hpp"

#include "App/Actions.hpp"
#include "App/Shortcuts.hpp"

#include "App/Popups.hpp"

#include "common.hpp"

#if !defined(__APPLE__)
#include "_binary/images/toastIcon.png.h"
#endif

App* gAppPtr{ nullptr };

void App::AttemptExit(bool force) {
    GET_SESSION_MANAGER;

    if (!force)
        for (unsigned i = 0; i < sessionManager.sessionList.size(); i++) {
            if (sessionManager.sessionList.at(i).modified) {
                Popups::_openExitWithChangesPopup = true;

                return; // Cancel
            }
        }

    GET_CONFIG_MANAGER;

    ConfigManager::Config config = configManager.getConfig();

    glfwGetWindowSize(window, &config.lastWindowWidth, &config.lastWindowHeight);

    configManager.setConfig(config);
    configManager.Save();

    this->running = false;
}

App::App() {
    gAppPtr = this;

    glfwSetErrorCallback([](int error_code, const char* description) {
        std::cerr << "GLFW Error (" << error_code << "): " << description << '\n';
    });
    if (UNLIKELY(!glfwInit())) {
        std::cerr << "[App:App] Failed to init GLFW!\n";
        __builtin_trap();
    }

    #if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glslVersion = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    #elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char* glslVersion = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    #else
        // GL 3.0 + GLSL 130
        const char* glslVersion = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    #endif

    // Config
    GET_CONFIG_MANAGER;
    configManager.Load();

    this->windowThreadID = std::this_thread::get_id();

    this->window = glfwCreateWindow(
        configManager.getConfig().lastWindowWidth,
        configManager.getConfig().lastWindowHeight,
        WINDOW_TITLE,
        nullptr, nullptr
    );

    if (UNLIKELY(!this->window)) {
        std::cerr << "[App::App] Window failed to init!\n";
        __builtin_trap();
    }

    glfwSetWindowCloseCallback(this->window, [](GLFWwindow* window) {
        extern App* gAppPtr;
        gAppPtr->AttemptExit();
    });

    glfwMakeContextCurrent(this->window);
    //glfwSwapInterval(1); // Enable vsync

    // Icon
#if !defined(__APPLE__)
    GLFWimage windowIcon;
    windowIcon.pixels = stbi_load_from_memory(toastIcon_png, toastIcon_png_size, &windowIcon.width, &windowIcon.height, nullptr, 4);

    glfwSetWindowIcon(window, 1, &windowIcon);
    stbi_image_free(windowIcon.pixels);
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    GET_IMGUI_IO;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    GET_APP_STATE;

    appState.applyTheming();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.f;
        style.Colors[ImGuiCol_WindowBg].w = 1.f;
    }

    style.WindowPadding.x = 10.f;
    style.WindowPadding.y = 10.f;

    style.FrameRounding = 5.f;
    style.WindowRounding = 5.f;
    style.ChildRounding = 5.f;
    style.GrabRounding = 5.f;
    style.PopupRounding = 5.f;

    style.FramePadding.x = 5.f;
    style.FramePadding.y = 3.f;

    style.ItemSpacing.x = 9.f;
    style.ItemSpacing.y = 4.f;

    style.WindowTitleAlign.x = .5f;
    style.WindowTitleAlign.y = .5f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    this->SetupFonts();
}

App::~App() {
    glfwMakeContextCurrent(this->window);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::SetupFonts() {
    GET_IMGUI_IO;
    GET_APP_STATE;

    { // SegoeUI (18px)
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fonts.normal = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 18.f, &fontConfig);
    }

    { // Font Awesome
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig fontConfig;
        fontConfig.MergeMode = true;
        fontConfig.PixelSnapH = true;

        appState.fonts.icon = io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_FA, 15.f, &fontConfig, icons_ranges);
    }

    { // SegoeUI (24px)
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fonts.large = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 24.f, &fontConfig);
    }

    { // SegoeUI (52px)
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fonts.giant = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 52.f, &fontConfig);
    }
}

// TODO: Move all of this outside of App.cpp

namespace ImGui {
    void EvalBezier(const float P[4], ImVec2* results, unsigned stepCount) {
        const float step = 1.f / stepCount;

        float t = step;
        for (unsigned i = 1; i <= stepCount; i++, t += step) {
            float u = 1.f - t;

            float t2 = t * t;

            float t3 = t2 * t;
            float b1 = 3.f * u * u * t;
            float b2 = 3.f * u * t2;

            results[i] = {
                b1 * P[0] + b2 * P[2] + t3,
                b1 * P[1] + b2 * P[3] + t3
            };
        }
    }

    ImVec2 BezierValue(const float x, const float P[4]) {
        const int STEPS = 256;

        ImVec2 closestPoint = { 0, 0 };
        float closestDistance = fabsf(x);

        const float step = 1.f / STEPS;

        float t = step;
        for (unsigned i = 1; i <= STEPS; i++, t += step) {
            float u = 1.f - t;

            float t2 = t * t;

            float t3 = t2 * t;
            float b1 = 3.f * u * u * t;
            float b2 = 3.f * u * t2;

            ImVec2 point = {
                b1 * P[0] + b2 * P[2] + t3,
                b1 * P[1] + b2 * P[3] + t3
            };

            float distance = fabsf(x - point.x);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestPoint = point;
            }
            else
                break;
        }

        return closestPoint;
    }

    float BezierValueY(const float x, const float P[4]) {
        return BezierValue(x, P).y;
    }

    bool Bezier(const char* label, float P[4], bool handlesEnabled = true) {
        const int LINE_SEGMENTS = 32; // Line segments in rendered curve
        const int CURVE_WIDTH = 4; // Curve line width

        const int LINE_WIDTH = 1; // Handles: connecting line width
        const int GRAB_RADIUS = 6; // Handles: circle radius
        const int GRAB_BORDER = 2; // Handles: circle border width

        const ImGuiStyle& style = GetStyle();
        const ImGuiIO& IO = GetIO();

        ImDrawList* drawList = GetWindowDrawList();
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems)
            return false;

        bool changed{ false };

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
        sprintf(buf, "%s##bInteract", label);

        ImGui::InvisibleButton(buf, outerBb.GetSize(),
            ImGuiButtonFlags_MouseButtonLeft
        );

        const bool interactionHovered     = ImGui::IsItemHovered();
        const bool interactionActive      = ImGui::IsItemActive();      // Held
        const bool interactionDeactivated = ImGui::IsItemDeactivated(); // Un-held

        // Dragging
        bool draggingLeft = interactionActive &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.f);

        RenderFrame( // Outer frame
            outerBb.Min, outerBb.Max,
            GetColorU32(ImGuiCol_FrameBg, .75f), true,
            style.FrameRounding
        );
        RenderFrame( // Inner frame
            bb.Min, bb.Max,
            GetColorU32(ImGuiCol_FrameBg, 1.f), true,
            style.FrameRounding
        );

        // Draw grid
        for (unsigned i = 0; i <= bb.GetWidth(); i += IM_ROUND(bb.GetWidth() / 4)) {
            drawList->AddLine(
                ImVec2(bb.Min.x + i, outerBb.Min.y),
                ImVec2(bb.Min.x + i, outerBb.Max.y),
                GetColorU32(ImGuiCol_TextDisabled)
            );
        }
        for (unsigned i = 0; i <= bb.GetHeight(); i += IM_ROUND(bb.GetHeight() / 4)) {
            drawList->AddLine(
                ImVec2(outerBb.Min.x, bb.Min.y + i),
                ImVec2(outerBb.Max.x, bb.Min.y + i),
                GetColorU32(ImGuiCol_TextDisabled)
            );
        }

        // Evaluate curve
        std::array<ImVec2, LINE_SEGMENTS + 1> results;
        EvalBezier(P, results.data(), LINE_SEGMENTS);

        // Drawing & handle drag behaviour
        {
            bool hoveredHandle[2]{ false };
            static int handleBeingDragged{ -1 };

            // Handle drag behaviour
            if (handlesEnabled) {
                for (unsigned i = 0; i < 2; i++) {
                    ImVec2 pos = ImVec2(
                        (P[i*2+0]) * bb.GetWidth() + bb.Min.x,
                        (1 - P[i*2+1]) * bb.GetHeight() + bb.Min.y
                    );

                    hoveredHandle[i] =
                        Common::IsMouseInRegion(pos, GRAB_RADIUS);

                    if (hoveredHandle[i] || handleBeingDragged == i)
                        SetTooltip("(%4.3f, %4.3f)", P[i*2+0], P[i*2+1]);

                    if (
                        draggingLeft &&
                        handleBeingDragged < 0 &&
                        hoveredHandle[i] != 0
                    )
                        handleBeingDragged = i;

                    if (handleBeingDragged == i) {
                        P[i*2+0] += GetIO().MouseDelta.x / bb.GetWidth();
                        P[i*2+1] -= GetIO().MouseDelta.y / bb.GetHeight();

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
                for (unsigned i = 0; i < LINE_SEGMENTS + 1; ++i ) {
                    points[i] = {
                        results[i+0].x * bb.GetWidth() + bb.Min.x,
                        (1 - results[i+0].y) * bb.GetHeight() + bb.Min.y
                    };
                }

                const ImColor color = GetStyle().Colors[ImGuiCol_PlotLines];
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
                    handleColA (ColorConvertFloat4ToU32({
                        1.00f, 0.00f, 0.75f, lumaA
                    })),
                    handleColB (ColorConvertFloat4ToU32({
                        0.00f, 0.75f, 1.00f, lumaB
                    }));

                ImVec2 p1 = ImVec2(
                    P[0] * bb.GetWidth() + bb.Min.x,
                    (1.f - P[1]) * bb.GetHeight() + bb.Min.y
                );
                ImVec2 p2 = ImVec2(
                    P[2] * bb.GetWidth() + bb.Min.x,
                    (1.f - P[3]) * bb.GetHeight() + bb.Min.y
                );

                drawList->AddLine(bb.GetBL(), p1, IM_COL32_WHITE, LINE_WIDTH);
                drawList->AddLine(bb.GetTR(), p2, IM_COL32_WHITE, LINE_WIDTH);

                drawList->AddCircleFilled(p1, GRAB_RADIUS, IM_COL32_WHITE);
                drawList->AddCircleFilled(p1, GRAB_RADIUS-GRAB_BORDER, handleColA);
                drawList->AddCircleFilled(p2, GRAB_RADIUS, IM_COL32_WHITE);
                drawList->AddCircleFilled(p2, GRAB_RADIUS-GRAB_BORDER, handleColB);
            }

            drawList->PopClipRect();
        }

        return changed;
    }
}

void App::Menubar() {
    GET_APP_STATE;
    GET_SESSION_MANAGER;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(WINDOW_TITLE)) {
            if (ImGui::MenuItem((char*)ICON_FA_SHARE_FROM_SQUARE " About " WINDOW_TITLE, "", nullptr)) {
                this->windowAbout.window.get()->open = true;
                ImGui::SetWindowFocus("About");
            }

            ImGui::Separator();

            if (ImGui::MenuItem((char*)ICON_FA_WRENCH " Config", "", nullptr)) {
                this->windowConfig.window.get()->open = true;
                ImGui::SetWindowFocus("Config");
            }

            ImGui::Separator();

            if (ImGui::MenuItem((char*)ICON_FA_DOOR_OPEN " Quit", SCS_EXIT, nullptr))
                this->AttemptExit();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_IMPORT " Open (szs)...",
                SCS_LAUNCH_OPEN_SZS_DIALOG,
                nullptr
            ))
                Actions::Dialog_CreateCompressedArcSession();

            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_IMPORT " Open (seperated)...",
                SCS_LAUNCH_OPEN_TRADITIONAL_DIALOG,
                nullptr
            ))
                Actions::Dialog_CreateTraditionalSession();

            ImGui::Separator();

            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_EXPORT " Save (szs)", SCS_SAVE_CURRENT_SESSION_SZS, false,
                    sessionManager.getSessionAvaliable() &&
                    !sessionManager.getCurrentSession()->traditionalMethod
            ))
                Actions::SaveCurrentSessionSzs();

            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_EXPORT " Save as (szs)...", SCS_LAUNCH_SAVE_AS_SZS_DIALOG, false,
                sessionManager.getSessionAvaliable()
            ))
                Actions::Dialog_SaveCurrentSessionAsSzs();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem(
                (char*)ICON_FA_ARROW_ROTATE_LEFT " Undo", SCS_UNDO, false,
                    sessionManager.getSessionAvaliable() &&
                    sessionManager.getCurrentSession()->canUndo()
            ))
                sessionManager.getCurrentSession()->undo();

            if (ImGui::MenuItem(
                (char*)ICON_FA_ARROW_ROTATE_RIGHT " Redo", SCS_REDO, false,
                    sessionManager.getSessionAvaliable() &&
                    sessionManager.getCurrentSession()->canRedo()
            ))
                sessionManager.getCurrentSession()->redo();

            ImGui::EndMenu();
        }

        const bool sessionAvaliable = sessionManager.currentSession >= 0;

        if (ImGui::BeginMenu("Cellanim", sessionAvaliable)) {
            if (ImGui::BeginMenu("Select")) {
                for (unsigned i = 0; i < sessionManager.getCurrentSession()->cellanims.size(); i++) {
                    const std::string& str = sessionManager.getCurrentSession()->cellanims.at(i).name;

                    std::ostringstream fmtStream;
                    fmtStream << std::to_string(i+1) << ". " << str.substr(0, str.size() - 6);

                    if (ImGui::MenuItem(
                        fmtStream.str().c_str(), nullptr,
                        sessionManager.getCurrentSession()->currentCellanim == i
                    )) {
                        if (sessionManager.getCurrentSession()->currentCellanim != i) {
                            sessionManager.getCurrentSession()->executeCommand(
                            std::make_shared<CommandSwitchCellanim>(
                                sessionManager.currentSession, i
                            ));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Optimize .."))
                appState.OpenGlobalPopup("###MOptimizeGlobal");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Spritesheets", sessionAvaliable)) {
            ImGui::MenuItem("Switch sheet ..");

            ImGui::Separator();

            ImGui::MenuItem("Remove unused sheets ..");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Animation", sessionAvaliable)) {
            ImGui::Text("Edit macro name ..");

            ImGui::Separator();

            ImGui::Text("Swap index ..");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Key",
            sessionAvaliable &&
            !sessionManager.getInstance().getCurrentSession()->arrangementMode
        )) {
            ImGui::Text(
                "Selected key (no. %u)",
                appState.globalAnimatable->getCurrentKeyIndex()
            );

            auto* key = appState.globalAnimatable->getCurrentKey();

            ImGui::Separator();

            if (ImGui::MenuItem("Make arrangement unique (duplicate)")) {
                auto newKey = *key;
                newKey.arrangementIndex =
                    CellanimHelpers::DuplicateArrangement(key->arrangementIndex);

                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandModifyAnimationKey>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentAnimationIndex(),
                    appState.globalAnimatable->getCurrentKeyIndex(),
                    newKey
                ));
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Interpolate ..",
                nullptr, nullptr,

                appState.globalAnimatable->getCurrentKeyIndex() + 1 <
                appState.globalAnimatable->getCurrentAnimation()->keys.size()
            ))
                appState.OpenGlobalPopup("MInterpolateKeys");

            ImGui::Separator();

            if (ImGui::BeginMenu("Move selected key ..")) {
                ImGui::MenuItem(".. up");
                ImGui::MenuItem(".. down");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Duplicate selected key ..")) {
                ImGui::MenuItem(".. before");
                ImGui::MenuItem(".. after");
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete selected key")) {
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandDeleteAnimationKey>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentAnimationIndex(),
                    appState.globalAnimatable->getCurrentKeyIndex()
                ));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Arrangement", sessionAvaliable)) {
            uint16_t arrangementIndex =
                appState.globalAnimatable->getCurrentKey()->arrangementIndex;

            ImGui::Text(
                "Current arrangement (no. %u)", arrangementIndex + 1
            );

            ImGui::Separator();

            if (ImGui::MenuItem("Transform as whole .."))
                appState.OpenGlobalPopup("MTransformArrangement");

            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandDeleteArrangement>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    arrangementIndex
                ));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Part",
            sessionAvaliable && appState.selectedPart >= 0
        )) {
            auto* part =
                &appState.globalAnimatable->getCurrentArrangement()
                ->parts.at(appState.selectedPart);

            ImGui::Text(
                "Selected part (no. %u)",
                appState.selectedPart + 1
            );

            ImGui::Separator();

            ImGui::MenuItem("Visible", nullptr, &part->editorVisible);
            ImGui::MenuItem("Locked", nullptr, &part->editorLocked);

            ImGui::Separator();

            if (ImGui::BeginMenu("Region")) {
                if (ImGui::MenuItem("Pad (Expand/Contract) .."))
                    appState.OpenGlobalPopup("MPadRegion");

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Set name (identifier) ..");

            ImGui::Separator();

            if (ImGui::MenuItem("Delete"))
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandDeleteArrangementPart>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                    appState.selectedPart
                ));

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            if (ImGui::MenuItem("Canvas", "", !this->windowCanvas.shy))
                this->windowCanvas.shy ^= true;

            if (ImGui::MenuItem("Animation- / Arrangement List", "", !this->windowHybridList.shy))
                this->windowHybridList.shy ^= true;

            if (ImGui::MenuItem("Inspector", "", !this->windowInspector.shy))
                this->windowInspector.shy ^= true;

            if (ImGui::MenuItem("Spritesheet", "", !this->windowSpritesheet.shy))
                this->windowSpritesheet.shy ^= true;

            if (ImGui::MenuItem("Timeline", "", !this->windowTimeline.shy))
                this->windowTimeline.shy ^= true;

            ImGui::Separator();

            ImGui::MenuItem("Dear ImGui demo window", "", &appState.enableDemoWindow);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    BEGIN_GLOBAL_POPUP;
    { // MInterpolateKeys
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15.f, 15.f });

        static bool lateOpen{ false };
        const bool active = ImGui::BeginPopup("MInterpolateKeys");

        GET_ANIMATABLE;

        bool condition = globalAnimatable && globalAnimatable->cellanim.get();

        static auto animationBackup{ RvlCellAnim::Animation{} };
        RvlCellAnim::Animation* currentAnimation{ nullptr };
        RvlCellAnim::AnimationKey* currentKey{ nullptr };
        RvlCellAnim::AnimationKey* nextKey{ nullptr };

        if (condition) {
            currentAnimation = globalAnimatable->getCurrentAnimation();
            currentKey = globalAnimatable->getCurrentKey();
            nextKey = currentKey + 1;

            // Does next key exist?
            condition =
                globalAnimatable->getCurrentKeyIndex() + 1 < currentAnimation->keys.size();
        }

        if (!active && lateOpen && condition) {
            *currentAnimation = animationBackup;
            PlayerManager::getInstance().clampCurrentKeyIndex();

            lateOpen = false;
        }

        if (active && condition) {
            if (!lateOpen) {
                animationBackup = *currentAnimation;
            }

            lateOpen = true;

            ImGui::SeparatorText("Interpolate keys");

            {
                static std::array<float, 4> v = { 0.f, 0.f, 1.f, 1.f };
                static bool customEasing{ false };

                static const char* easeNames[] = {
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
                static const std::array<float, 4> easePresets[] = {
                    { 0.000f, 0.000f, 1.000f, 1.000f },
                    { 0.470f, 0.000f, 0.745f, 0.715f },
                    { 0.390f, 0.575f, 0.565f, 1.000f },
                    { 0.445f, 0.050f, 0.550f, 0.950f },
                    { 0.550f, 0.085f, 0.680f, 0.530f },
                    { 0.250f, 0.460f, 0.450f, 0.940f },
                    { 0.455f, 0.030f, 0.515f, 0.955f },
                    { 0.550f, 0.055f, 0.675f, 0.190f },
                    { 0.215f, 0.610f, 0.355f, 1.000f },
                    { 0.645f, 0.045f, 0.355f, 1.000f },
                    { 0.895f, 0.030f, 0.685f, 0.220f },
                    { 0.165f, 0.840f, 0.440f, 1.000f },
                    { 0.770f, 0.000f, 0.175f, 1.000f },
                    { 0.755f, 0.050f, 0.855f, 0.060f },
                    { 0.230f, 1.000f, 0.320f, 1.000f },
                    { 0.860f, 0.000f, 0.070f, 1.000f },
                    { 0.950f, 0.050f, 0.795f, 0.035f },
                    { 0.190f, 1.000f, 0.220f, 1.000f },
                    { 1.000f, 0.000f, 0.000f, 1.000f },
                    { 0.600f, 0.040f, 0.980f, 0.335f },
                    { 0.075f, 0.820f, 0.165f, 1.000f },
                    { 0.785f, 0.135f, 0.150f, 0.860f },
                    { 0.600f, -0.28f, 0.735f, 0.045f },
                    { 0.175f, 0.885f, 0.320f, 1.275f },
                    { 0.680f, -0.55f, 0.265f, 1.550f },
                    { 0.445f, 0.050f, 0.550f, 0.950f },
                };

                int selectedEaseIndex{ 0 };
                if (customEasing)
                    selectedEaseIndex = ARRAY_LENGTH(easePresets) - 1;
                else {
                    const std::array<float, 4>* easeSearch =
                        std::find(easePresets, easePresets + ARRAY_LENGTH(easePresets) - 1, v);
                    selectedEaseIndex = easeSearch - easePresets;
                }

                ImGui::Bezier("CurveView", v.data(), customEasing);

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

                ImGui::EndGroup();

                float t = fmodf((float)ImGui::GetTime(), 1.f);

                ImGui::ProgressBar(ImGui::BezierValueY(t, v.data()));
            }

            ImGui::Separator();

            if (ImGui::Button("Apply")) {


                SessionManager::getInstance().getCurrentSessionModified() = true;

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
    END_GLOBAL_POPUP;

    if (ImGui::BeginMenuBar()) {
        ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll;

        GET_SESSION_MANAGER;

        if (ImGui::BeginTabBar("FileTabBar", tabBarFlags)) {
            for (unsigned n = 0; n < sessionManager.sessionList.size(); n++) {
                ImGui::PushID(n);

                bool sessionOpen{ true };
                if (
                    ImGui::BeginTabItem(
                        sessionManager.sessionList.at(n).mainPath.c_str(),
                        &sessionOpen,
                        ImGuiTabItemFlags_None | (
                            sessionManager.sessionList.at(n).modified ?
                            ImGuiTabItemFlags_UnsavedDocument : 0
                        )
                    )
                ) {
                    if (sessionManager.currentSession != n) {
                        sessionManager.currentSession = n;
                        sessionManager.SessionChanged();
                    }

                    ImGui::EndTabItem();

                    if (ImGui::BeginPopupContextItem()) {
                        ImGui::Text("Select a Cellanim:");
                        ImGui::Separator();
                        for (unsigned i = 0; i < sessionManager.sessionList.at(n).cellanims.size(); i++) {
                            const std::string& str = sessionManager.sessionList.at(n).cellanims.at(i).name;

                            std::ostringstream fmtStream;
                            fmtStream << std::to_string(i+1) << ". " << str.substr(0, str.size() - 6);

                            if (ImGui::MenuItem(
                                fmtStream.str().c_str(), nullptr,
                                sessionManager.sessionList.at(n).currentCellanim == i
                            )) {
                                ImGui::CloseCurrentPopup();

                                if (sessionManager.sessionList.at(n).currentCellanim != i) {
                                    sessionManager.currentSession = n;

                                    sessionManager.sessionList.at(n).executeCommand(
                                    std::make_shared<CommandSwitchCellanim>(
                                        n, i
                                    ));
                                }
                            }
                        }

                        ImGui::EndPopup();
                    }
                }

                const std::string& cellanimName = sessionManager.sessionList.at(n).cellanims.at(
                    sessionManager.sessionList.at(n).currentCellanim
                ).name;
                ImGui::SetItemTooltip(
                    "Path: %s\nCellanim: %s\n\nRight-click to select the cellanim.",
                    sessionManager.sessionList.at(n).mainPath.c_str(),
                    cellanimName.substr(0, cellanimName.size() - 6).c_str()
                );

                if (!sessionOpen) {
                    sessionManager.sessionClosing = n;

                    if (sessionManager.sessionList.at(n).modified)
                        AppState::getInstance().OpenGlobalPopup("###CloseModifiedSession");
                    else {
                        sessionManager.FreeSessionIndex(n);
                        sessionManager.SessionChanged();
                    }
                }

                ImGui::PopID();
            }
            ImGui::EndTabBar();
        }

        ImGui::EndMenuBar();
    }
}

void App::Update() {
    GET_APP_STATE;

    // Update rate
    {
        static std::chrono::system_clock::time_point a{ std::chrono::system_clock::now() };
        static std::chrono::system_clock::time_point b{ std::chrono::system_clock::now() };

        a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> workTime = a - b;

        double updateRate = 1000. / ConfigManager::getInstance().getConfig().updateRate;

        if (workTime.count() < updateRate) {
            std::chrono::duration<double, std::milli> deltaMs(updateRate - workTime.count());
            auto deltaMsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(deltaMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(deltaMsDuration.count()));
        }

        b = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> sleep_time = b - a;
    }

    static ImGuiIO& io{ ImGui::GetIO() };

    glfwMakeContextCurrent(this->window);

    glfwPollEvents();

    // Start frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Begin main window
    {
        static const ImGuiDockNodeFlags dockspaceFlags =
            ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiWindowFlags_NoBackground;
        static const ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f, 0.f, 0.f, 0.f });

        ImGui::Begin(WINDOW_TITLE "###AppWindow", nullptr, windowFlags);

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();

        // Submit the Dockspace
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
            ImGui::DockSpace(ImGui::GetID("mainDockspace"), { 0.f, 0.f }, dockspaceFlags);
    }

    this->Menubar();

    Shortcuts::Handle();

    if (appState.enableDemoWindow)
        ImGui::ShowDemoWindow(&appState.enableDemoWindow);

    if (SessionManager::getInstance().getSessionAvaliable()) {
        PlayerManager::getInstance().Update();

        // Windows
        this->windowCanvas.Update();
        this->windowHybridList.Update();
        this->windowInspector.Update();
        this->windowTimeline.Update();
        this->windowSpritesheet.Update();
    }

    this->windowConfig.Update();
    this->windowAbout.Update();

    AsyncTaskManager::getInstance().UpdateTasks();

    MtCommandManager::getInstance().Update();

    Popups::Update();

    // End main window
    ImGui::End();

    // Render
    ImGui::Render();

    int framebufferSize[2];
    glfwGetFramebufferSize(this->window, framebufferSize + 0, framebufferSize + 1);

    glViewport(0, 0, framebufferSize[0], framebufferSize[1]);

    const auto clearColor = appState.getWindowClearColor();
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        glfwMakeContextCurrent(backupCurrentContext);
    }

    glfwSwapBuffers(this->window);
}
