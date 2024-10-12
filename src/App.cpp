#include "App.hpp"

#include <algorithm>
#include <array>

#include <cmath>

#include <imgui.h>

#include <chrono>

#include <iostream>

#include <string>
#include <sstream>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
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
#include "command/CommandDeleteArrangementPart.hpp"
#include "command/CommandModifyAnimationKey.hpp"
#include "command/CommandDeleteAnimationKey.hpp"
#include "command/CommandMoveAnimationKey.hpp"
#include "command/CommandInsertAnimationKey.hpp"

#include "anim/CellanimHelpers.hpp"

#include "task/AsyncTaskManager.hpp"

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
            if (sessionManager.sessionList[i].modified) {
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

    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "GLFW Error (" << code << "): " << description << '\n';
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

    this->mainThreadId = std::this_thread::get_id();

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

void App::Menubar() {
    GET_APP_STATE;
    GET_SESSION_MANAGER;
    GET_PLAYER_MANAGER;
    GET_CONFIG_MANAGER;

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

            const auto& recentlyOpened = configManager.getConfig().recentlyOpened;
            if (ImGui::BeginMenu(
                (char*)ICON_FA_FILE_IMPORT " Open recent",
                !recentlyOpened.empty()
            )) {
                for (unsigned i = 0; i < 12; i++) {
                    const int j = recentlyOpened.size() > i ? recentlyOpened.size() - 1 - i : -1;
                    const bool none = (j < 0);

                    ImGui::BeginDisabled(none);

                    char label[128];
                    if (none)
                        snprintf(label, 128, "%u. -", i + 1);
                    else
                        snprintf(label, 128, "%u. %s", i + 1, recentlyOpened[j].c_str());

                    if (ImGui::MenuItem(label) && !none) {
                        AsyncTaskManager::getInstance().StartTask<PushSessionTask>(
                            recentlyOpened[j]
                        );
                    }

                    ImGui::EndDisabled();
                }

                ImGui::EndMenu();
            }

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
            const unsigned animIndex =
                appState.globalAnimatable->getCurrentAnimationIndex();

            ImGui::Text("Selected animation (no. %u)", animIndex + 1);

            ImGui::Separator();

            if (ImGui::MenuItem("Edit macro name ..")) {
                Popups::_editAnimationNameIdx = animIndex;
                appState.OpenGlobalPopup("###EditAnimationName");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Swap index ..")) {
                Popups::_swapAnimationIdx = animIndex;
                appState.OpenGlobalPopup("###SwapAnimation");
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Key",
            sessionAvaliable &&
            !sessionManager.getInstance().getCurrentSession()->arrangementMode &&
            !playerManager.playing
        )) {
            const unsigned keyIndex =
                appState.globalAnimatable->getCurrentKeyIndex();
            auto* key = appState.globalAnimatable->getCurrentKey();

            ImGui::Text(
                "Selected key (no. %u)",
                keyIndex + 1
            );

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Interpolate ..",
                nullptr, nullptr,

                keyIndex + 1 <
                appState.globalAnimatable->getCurrentAnimation()->keys.size()
            ))
                appState.OpenGlobalPopup("MInterpolateKeys");

            ImGui::Separator();

            if (ImGui::BeginMenu("Move selected key ..")) {
                ImGui::BeginDisabled(keyIndex == (playerManager.getKeyCount() - 1));
                if (ImGui::MenuItem(".. up")) {
                    sessionManager.getCurrentSession()->executeCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentAnimationIndex(),
                        keyIndex,
                        false,
                        false
                    ));

                    sessionManager.getCurrentSessionModified() = true;

                    playerManager.setCurrentKeyIndex(keyIndex + 1);
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(keyIndex == 0);
                if (ImGui::MenuItem(".. down")) {
                    sessionManager.getCurrentSession()->executeCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentAnimationIndex(),
                        keyIndex,
                        true,
                        false
                    ));

                    sessionManager.getCurrentSessionModified() = true;

                    playerManager.setCurrentKeyIndex(keyIndex - 1);
                }
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Duplicate selected key ..")) {
                if (ImGui::MenuItem(".. after")) {
                    sessionManager.getCurrentSession()->executeCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentAnimationIndex(),
                        keyIndex + 1,
                        *key
                    ));

                    playerManager.setCurrentKeyIndex(keyIndex + 1);

                    sessionManager.getCurrentSessionModified() = true;
                }

                if (ImGui::MenuItem(".. before")) {
                    sessionManager.getCurrentSession()->executeCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentAnimationIndex(),
                        keyIndex,
                        *key
                    ));

                    playerManager.setCurrentKeyIndex(keyIndex);

                    sessionManager.getCurrentSessionModified() = true;
                }

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

                sessionManager.getCurrentSessionModified() = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Arrangement",
            sessionAvaliable &&
            !playerManager.playing
        )) {
            const unsigned arrangementIndex =
                appState.globalAnimatable->getCurrentKey()->arrangementIndex;

            ImGui::Text(
                "Current arrangement (no. %u)",
                arrangementIndex + 1
            );

            ImGui::Separator();

            // Duplicate option
            {
                const unsigned keyIndex =
                    appState.globalAnimatable->getCurrentKeyIndex();
                auto* key = appState.globalAnimatable->getCurrentKey();

                bool arrangementUnique = CellanimHelpers::getArrangementUnique(key->arrangementIndex);
                ImGui::BeginDisabled(arrangementUnique);

                if (ImGui::MenuItem("Make arrangement unique (duplicate)")) {
                    unsigned newIndex = CellanimHelpers::DuplicateArrangement(key->arrangementIndex);

                    if (!appState.getArrangementMode()) {
                        auto newKey = *key;
                        newKey.arrangementIndex = newIndex;

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyAnimationKey>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentAnimationIndex(),
                            keyIndex,
                            newKey
                        ));
                    }
                    else
                        key->arrangementIndex = newIndex;
                }

                ImGui::EndDisabled();

                if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("This arrangement is only used once.");
            }

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

                sessionManager.getCurrentSessionModified() = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Part",
            sessionAvaliable &&
            appState.selectedPart >= 0 &&
            !playerManager.playing
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

            if (ImGui::MenuItem("Delete")) {
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandDeleteArrangementPart>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                    appState.selectedPart
                ));

                sessionManager.getCurrentSessionModified() = true;
            }

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

            if (ImGui::MenuItem("Dear ImGui demo window", "", this->windowDemo.window.get()->open))
                this->windowDemo.window.get()->open ^= true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMenuBar()) {
        static const ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll |
            ImGuiTabBarFlags_AutoSelectNewTabs;

        GET_SESSION_MANAGER;

        if (ImGui::BeginTabBar("FileTabBar", tabBarFlags)) {
            for (unsigned n = 0; n < sessionManager.sessionList.size(); n++) {
                ImGui::PushID(n);

                bool sessionOpen{ true };
                bool tabVisible = ImGui::BeginTabItem(
                    sessionManager.sessionList.at(n).mainPath.c_str(),
                    &sessionOpen,
                    ImGuiTabItemFlags_None | (
                        sessionManager.sessionList.at(n).modified ?
                        ImGuiTabItemFlags_UnsavedDocument : 0
                    )
                );

                if (tabVisible)
                    ImGui::EndTabItem();

                if (ImGui::IsItemClicked() && sessionManager.currentSession != n) {
                    sessionManager.currentSession = n;
                    sessionManager.SessionChanged();
                }

                if (tabVisible && ImGui::BeginPopupContextItem()) {
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

    this->windowDemo.Update();

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

    const auto& clearColor = appState.getWindowClearColor();
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
