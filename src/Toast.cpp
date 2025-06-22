#include "Toast.hpp"

#include <cstddef>

#include <chrono>

#include <iostream>

#include <string>
#include <sstream>

#include <array>

#include <imgui.h>

#include <tinyfiledialogs.h>

#include "cellanim/CellAnim.hpp"

#include "manager/AppState.hpp"

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/PlayerManager.hpp"
#include "manager/ThemeManager.hpp"
#include "manager/MainThreadTaskManager.hpp"

#include "font/FontAwesome.h"

#include "command/CommandSwitchCellanim.hpp"
#include "command/CommandDeleteArrangement.hpp"
#include "command/CommandDeleteArrangementPart.hpp"
#include "command/CommandModifyAnimationKey.hpp"
#include "command/CommandDeleteAnimationKey.hpp"
#include "command/CommandMoveAnimationKey.hpp"
#include "command/CommandInsertAnimationKey.hpp"

#include "manager/AsyncTaskManager.hpp"

#include "task/AsyncTaskPushSession.hpp"

#include "App/Actions.hpp"
#include "App/Shortcuts.hpp"

#include "App/Popups.hpp"

// macOS doesn't support assigning a window icon
#if !defined(__APPLE__)
#include "stb/stb_image.h"

#include "BIN/image/toastIcon.png.h"
#endif // !defined(__APPLE__)

#define WINDOW_TITLE "toast"

Toast* Toast::gInstance { nullptr };
Toast* Toast::getInstance() {
    if (gInstance == nullptr)
        throw std::runtime_error("Toast:getInstance: Instance of Toast does not exist!");
    return gInstance;
}

static void syncToUpdateRate() {
    static std::chrono::system_clock::time_point a { std::chrono::system_clock::now() };
    static std::chrono::system_clock::time_point b { std::chrono::system_clock::now() };

    a = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> workTime = a - b;

    double updateRate = 1000. / ConfigManager::getInstance().getConfig().updateRate;

    if (workTime.count() < updateRate) {
        std::chrono::duration<double, std::milli> deltaMs(updateRate - workTime.count());
        std::chrono::milliseconds deltaMsDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(deltaMs);

        std::this_thread::sleep_for(std::chrono::milliseconds(deltaMsDuration.count()));
    }

    b = std::chrono::system_clock::now();
}

Toast::Toast(int argc, const char** argv) {
    if (gInstance != nullptr)
        throw std::runtime_error("Toast:Toast: Instance of Toast already exists!");
    gInstance = this;

    mMainThreadId = std::this_thread::get_id();

    Logging::Open("toast.log");

    glfwSetErrorCallback([](int code, const char* message) {
        Logging::err << "GLFW threw an error (code " << code << "): " << message << std::endl;
    });

    if (!glfwInit())
        throw std::runtime_error("Toast:Toast: Failed to init GLFW!");

    MainThreadTaskManager::createSingleton();
    AsyncTaskManager::createSingleton();
    AppState::createSingleton();
    ThemeManager::createSingleton();
    ConfigManager::createSingleton();
    PlayerManager::createSingleton();
    SessionManager::createSingleton();

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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
    // GL 3.0 + GLSL 130
    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only
#endif // defined(__APPLE__), defined(IMGUI_IMPL_OPENGL_ES2)

    ConfigManager& configManager = ConfigManager::getInstance();
    configManager.LoadConfig();

    int windowWidth = configManager.getConfig().lastWindowWidth;
    int windowHeight = configManager.getConfig().lastWindowHeight;

    if (windowWidth < 1)
        windowWidth = Config::DEFAULT_WINDOW_WIDTH;
    if (windowHeight < 1)
        windowHeight = Config::DEFAULT_WINDOW_HEIGHT;

    mGlfwWindowHndl = glfwCreateWindow(
        windowWidth, windowHeight,
        WINDOW_TITLE,
        nullptr, nullptr
    );

    if (!mGlfwWindowHndl) {
        glfwTerminate();
        throw std::runtime_error("Toast::Toast: glfwCreateWindow failed!");
    }

    glfwSetWindowCloseCallback(mGlfwWindowHndl, [](GLFWwindow*) {
        Toast::getInstance()->AttemptExit();
    });

    mIsWindowMaximized = configManager.getConfig().lastWindowMaximized;
    if (mIsWindowMaximized)
        glfwMaximizeWindow(mGlfwWindowHndl);

    glfwSetWindowMaximizeCallback(mGlfwWindowHndl, [](GLFWwindow*, int maximized) {
        Toast::getInstance()->mIsWindowMaximized = maximized;
    });

    glfwMakeContextCurrent(mGlfwWindowHndl);
    glfwSwapInterval(1); // Enable VSync.

#if defined(USING_GLEW)
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(mGlfwWindowHndl);
        glfwTerminate();

        throw std::runtime_error("Toast::Toast: Failed to init GLFW!");
    }
#endif

    // macOS doesn't support assigning a window icon
#if !defined(__APPLE__)
    GLFWimage windowIcon;
    windowIcon.pixels = stbi_load_from_memory(toastIcon_png, toastIcon_png_size, &windowIcon.width, &windowIcon.height, nullptr, 4);

    // glfwSetWindowIcon copies the image data.
    glfwSetWindowIcon(mGlfwWindowHndl, 1, &windowIcon);

    stbi_image_free(windowIcon.pixels);
#endif // !defined(__APPLE__)

#if !defined(NDEBUG)
    IMGUI_CHECKVERSION();
#endif // !defined(NDEBUG)

    mGuiContext = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags =
        ImGuiConfigFlags_NavEnableKeyboard | // Enable Keyboard Controls
        ImGuiConfigFlags_DockingEnable |     // Enable Docking
        ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

    ImGui_ImplGlfw_InitForOpenGL(mGlfwWindowHndl, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    ThemeManager& themeManager = ThemeManager::getInstance();

    themeManager.Initialize();

    themeManager.applyTheming();

    CellAnimRenderer::InitShader();

    glfwSetDropCallback(mGlfwWindowHndl, [](GLFWwindow*, int pathCount, const char** paths) {
        for (int i = 0; i < pathCount; i++) {
            AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(
                std::string(paths[i])
            );
        }
    });

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(
                std::string(argv[i])
            );
        }
    }

    Logging::info << "[Toast::Toast] Finished initializing." << std::endl;
}

Toast::~Toast() {
    glfwMakeContextCurrent(mGlfwWindowHndl);

    mWindowCanvas.Destroy();
    mWindowHybridList.Destroy();
    mWindowInspector.Destroy();
    mWindowTimeline.Destroy();
    mWindowSpritesheet.Destroy();

    mWindowConfig.Destroy();
    mWindowAbout.Destroy();

    mWindowDemo.Destroy();

    SessionManager::getInstance().RemoveAllSessions();
    SessionManager::destroySingleton();

    CellAnimRenderer::DestroyShader();

    AppState::destroySingleton();
    ConfigManager::destroySingleton();
    PlayerManager::destroySingleton();
    ThemeManager::destroySingleton();
    AsyncTaskManager::destroySingleton();
    MainThreadTaskManager::destroySingleton();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(mGlfwWindowHndl);
    glfwTerminate();

    Logging::info << "[Toast::Toast] Finished deinitializing." << std::endl;

    Logging::Close();

    gInstance = nullptr;
}

void Toast::AttemptExit(bool force) {
    if (!force) {
        for (const auto& session : SessionManager::getInstance().getSessions()) {
            if (session.modified) {
                Popups::_openExitWithChangesPopup = true;
                return; // Cancel
            }
        }
    }

    ConfigManager& configManager = ConfigManager::getInstance();

    Config config = configManager.getConfig();

    glfwGetWindowSize(mGlfwWindowHndl, &config.lastWindowWidth, &config.lastWindowHeight);
    config.lastWindowMaximized = mIsWindowMaximized;

    configManager.setConfig(config);
    configManager.SaveConfig();

    mRunning = false;
}

void Toast::Menubar() {
    const AppState& appState = AppState::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();
    ConfigManager& configManager = ConfigManager::getInstance();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(WINDOW_TITLE)) {
            if (ImGui::MenuItem((const char*)ICON_FA_SHARE_FROM_SQUARE " About " WINDOW_TITLE, "", nullptr)) {
                mWindowAbout.windowInstance->mOpen = true;
                ImGui::SetWindowFocus("About");
            }

            ImGui::Separator();

            if (ImGui::MenuItem((const char*)ICON_FA_WRENCH " Config", "", nullptr)) {
                mWindowConfig.windowInstance->mOpen = true;
                ImGui::SetWindowFocus("Config");
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Exit, ICON_FA_DOOR_OPEN).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Exit).c_str()
            ))
                AttemptExit();

            ImGui::EndMenu();
        }

        CellAnim::CellAnimType cellanimType { CellAnim::CELLANIM_TYPE_INVALID };
            if (sessionManager.getSessionAvaliable())
                cellanimType = sessionManager.getCurrentSession()->type;

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Open, ICON_FA_FILE_IMPORT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Open).c_str(),
                nullptr
            ))
                Actions::CreateSessionPromptPath();

            const auto& recentlyOpened = configManager.getConfig().recentlyOpened;

            if (ImGui::BeginMenu(
                (const char*)ICON_FA_FILE_IMPORT " Open recent",
                !recentlyOpened.empty()
            )) {
                for (size_t i = 0; i < Config::MAX_RECENTLY_OPENED; i++) {
                    const int j = recentlyOpened.size() > i ? recentlyOpened.size() - 1 - i : -1;
                    const bool none = (j < 0);

                    char label[128];
                    snprintf(label, sizeof(label), "%zu. %s", i + 1, none ? "-" : recentlyOpened[j].c_str());

                    ImGui::BeginDisabled(none);

                    if (ImGui::MenuItem(label) && !none) {
                        AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(
                            recentlyOpened[j]
                        );
                    }

                    ImGui::EndDisabled();
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                (const char*)ICON_FA_FILE_IMPORT " Open containing folder...",
                nullptr, false, sessionManager.getSessionAvaliable()
            ))
                Actions::OpenSessionSourceFolder();

            ImGui::Separator();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Save, ICON_FA_FILE_EXPORT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Save).c_str(),
                false,
                sessionManager.getSessionAvaliable()
            ))
                Actions::ExportSession();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::SaveAs, ICON_FA_FILE_EXPORT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::SaveAs).c_str(),
                false,
                sessionManager.getSessionAvaliable()
            ))
                Actions::ExportSessionPromptPath();

            ImGui::Separator();

            const char* convertOptionTitle = (const char*)ICON_FA_FILE_EXPORT " Save as other...";
            if (cellanimType == CellAnim::CELLANIM_TYPE_RVL)
                convertOptionTitle = (const char*)ICON_FA_FILE_EXPORT " Save as Megamix (CTR) cellanim...";
            else if (cellanimType == CellAnim::CELLANIM_TYPE_CTR)
                convertOptionTitle = (const char*)ICON_FA_FILE_EXPORT " Save as Fever (RVL) cellanim...";

            if (ImGui::MenuItem(
                convertOptionTitle, nullptr, false, sessionManager.getSessionAvaliable()
            ))
                Actions::ExportSessionAsOther();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Undo, ICON_FA_ARROW_ROTATE_LEFT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Undo).c_str(),
                false,
                sessionManager.getSessionAvaliable() &&
                sessionManager.getCurrentSession()->canUndo()
            ))
                sessionManager.getCurrentSession()->undo();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Redo, ICON_FA_ARROW_ROTATE_RIGHT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Redo).c_str(),
                false,
                sessionManager.getSessionAvaliable() &&
                sessionManager.getCurrentSession()->canRedo()
            ))
                sessionManager.getCurrentSession()->redo();

            ImGui::EndMenu();
        }

        const bool sessionAvaliable = sessionManager.getSessionAvaliable();

        if (ImGui::BeginMenu("Cellanim", sessionAvaliable)) {
            if (ImGui::BeginMenu("Select")) {
                auto* currentSession = sessionManager.getCurrentSession();

                for (size_t i = 0; i < currentSession->cellanims.size(); i++) {
                    std::ostringstream fmtStream;
                    fmtStream << std::to_string(i+1) << ". " << currentSession->cellanims[i].name;

                    if (ImGui::MenuItem(
                        fmtStream.str().c_str(), nullptr,
                        currentSession->getCurrentCellAnimIndex() == i
                    )) {
                        if (currentSession->getCurrentCellAnimIndex() != i) {
                            currentSession->addCommand(
                            std::make_shared<CommandSwitchCellanim>(
                                sessionManager.getCurrentSessionIndex(), i
                            ));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Optimize .."))
                OPEN_GLOBAL_POPUP("###MOptimizeGlobal");

            ImGui::Separator();

            if (ImGui::MenuItem("Transform .."))
                OPEN_GLOBAL_POPUP("MTransformCellanim");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Spritesheets", sessionAvaliable)) {
            if (ImGui::MenuItem("Open spritesheet manager .."))
                OPEN_GLOBAL_POPUP("###SpritesheetManager");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Animation", sessionAvaliable)) {
            const unsigned animIndex = playerManager.getAnimationIndex();

            ImGui::Text("Selected animation (no. %u)", animIndex + 1);

            ImGui::Separator();

            if (ImGui::MenuItem("Edit name ..")) {
                Popups::_editAnimationNameIdx = animIndex;
                OPEN_GLOBAL_POPUP("###EditAnimationName");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Swap index ..")) {
                Popups::_swapAnimationIdx = animIndex;
                OPEN_GLOBAL_POPUP("###SwapAnimation");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Transform .."))
                OPEN_GLOBAL_POPUP("MTransformAnimation");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Key",
            sessionAvaliable &&
            !sessionManager.getInstance().getCurrentSession()->arrangementMode &&
            !playerManager.getPlaying()
        )) {
            const unsigned keyIndex = playerManager.getKeyIndex();
            const auto& key = playerManager.getKey();

            ImGui::Text(
                "Selected key (no. %u)",
                keyIndex + 1
            );

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Tween ..",
                nullptr, nullptr,
                keyIndex + 1 < playerManager.getKeyCount()
            ))
                OPEN_GLOBAL_POPUP("MInterpolateKeys");

            ImGui::Separator();

            if (ImGui::BeginMenu("Move selected key ..")) {
                ImGui::BeginDisabled(keyIndex == (playerManager.getKeyCount() - 1));
                if (ImGui::MenuItem(".. up")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex,
                        false,
                        false
                    ));

                    playerManager.setKeyIndex(keyIndex + 1);
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(keyIndex == 0);
                if (ImGui::MenuItem(".. down")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex,
                        true,
                        false
                    ));

                    playerManager.setKeyIndex(keyIndex - 1);
                }
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Duplicate selected key ..")) {
                if (ImGui::MenuItem(".. after")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex + 1,
                        key
                    ));

                    playerManager.setKeyIndex(keyIndex + 1);
                }

                if (ImGui::MenuItem(".. before")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex,
                        key
                    ));

                    playerManager.setKeyIndex(keyIndex);
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete selected key")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteAnimationKey>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getAnimationIndex(),
                    playerManager.getKeyIndex()
                ));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Arrangement",
            sessionAvaliable &&
            !playerManager.getPlaying()
        )) {
            const unsigned arrangementIndex = playerManager.getArrangementIndex();

            ImGui::Text(
                "Current arrangement (no. %u)",
                arrangementIndex + 1
            );

            ImGui::Separator();

            // Duplicate option
            {
                auto& cellAnim = *sessionManager.getCurrentSession()->getCurrentCellAnim().object;

                const unsigned keyIndex = playerManager.getKeyIndex();
                auto& key = playerManager.getKey();

                bool arrangementUnique = cellAnim.isArrangementUnique(arrangementIndex);
                ImGui::BeginDisabled(arrangementUnique);

                if (ImGui::MenuItem("Make arrangement unique (duplicate)")) {
                    unsigned index = cellAnim.duplicateArrangement(key.arrangementIndex);

                    if (!appState.getArrangementMode()) {
                        auto newKey = key;
                        newKey.arrangementIndex = index;

                        sessionManager.getCurrentSession()->addCommand(
                        std::make_shared<CommandModifyAnimationKey>(
                            sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                            playerManager.getAnimationIndex(),
                            keyIndex,
                            newKey
                        ));
                    }
                    else
                        key.arrangementIndex = index;
                }

                ImGui::EndDisabled();

                if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("This arrangement is only used once.");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Transform as whole .."))
                OPEN_GLOBAL_POPUP("MTransformArrangement");

            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteArrangement>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    arrangementIndex
                ));
            }

            ImGui::EndMenu();
        }

        bool singlePartSelected = false;
        if (sessionAvaliable) {
            const auto& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();
            singlePartSelected = selectionState.singlePartSelected();
        }

        if (ImGui::BeginMenu("Part",
            sessionAvaliable &&
            singlePartSelected &&
            !playerManager.getPlaying()
        )) {
            const auto& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

            auto& part = playerManager.getArrangement().parts.at(selectionState.mSelectedParts[0].index);

            ImGui::Text(
                "Selected part (no. %u)",
                selectionState.mSelectedParts[0].index + 1
            );

            ImGui::Separator();

            ImGui::MenuItem("Visible", nullptr, &part.editorVisible);
            ImGui::MenuItem("Locked", nullptr, &part.editorLocked);

            ImGui::Separator();

            if (ImGui::BeginMenu("Cell")) {
                if (ImGui::MenuItem("Pad (Expand/Contract) .."))
                    OPEN_GLOBAL_POPUP("MPadRegion");

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Set editor name ..")) {
                Popups::_editPartNameArrangeIdx =
                    playerManager.getArrangementIndex();
                Popups::_editPartNamePartIdx = selectionState.mSelectedParts[0].index;

                OPEN_GLOBAL_POPUP("###EditPartName");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteArrangementPart>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getArrangementIndex(),
                    selectionState.mSelectedParts[0].index
                ));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            mWindowCanvas.UIShyToggle("Canvas");
            mWindowHybridList.UIShyToggle("Animation / Arrangement List");
            mWindowInspector.UIShyToggle("Inspector");
            mWindowSpritesheet.UIShyToggle("Spritesheet");
            mWindowTimeline.UIShyToggle("Timeline");

            ImGui::Separator();

            if (ImGui::MenuItem("Dear ImGui demo window", "", mWindowDemo.windowInstance->mOpen))
                mWindowDemo.windowInstance->mOpen ^= true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMenuBar()) {
        constexpr ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll |
            ImGuiTabBarFlags_AutoSelectNewTabs;

        SessionManager& sessionManager = SessionManager::getInstance();

        if (ImGui::BeginTabBar("SessionTabs", tabBarFlags)) {
            for (int n = 0; n < static_cast<int>(sessionManager.getSessionCount()); n++) {
                ImGui::PushID(n);

                auto& session = sessionManager.getSession(n);

                bool sessionOpen { true };
                bool tabVisible = ImGui::BeginTabItem(
                    session.resourcePath.c_str(),
                    &sessionOpen,
                    session.modified ?
                        ImGuiTabItemFlags_UnsavedDocument :
                        ImGuiTabItemFlags_None
                );

                if (tabVisible)
                    ImGui::EndTabItem();

                if (ImGui::IsItemClicked() && sessionManager.getCurrentSessionIndex() != n) {
                    sessionManager.setCurrentSessionIndex(n);
                }

                if (tabVisible && ImGui::BeginPopupContextItem()) {
                    ImGui::TextUnformatted("Select a Cellanim:");
                    ImGui::Separator();
                    for (size_t i = 0; i < session.cellanims.size(); i++) {
                        std::ostringstream fmtStream;
                        fmtStream << std::to_string(i+1) << ". " << session.cellanims[i].name;

                        if (ImGui::MenuItem(
                            fmtStream.str().c_str(), nullptr,
                            session.getCurrentCellAnimIndex() == i
                        )) {
                            ImGui::CloseCurrentPopup();

                            if (session.getCurrentCellAnimIndex() != i) {
                                sessionManager.setCurrentSessionIndex(n);

                                // TODO: bug here probably
                                session.addCommand(
                                std::make_shared<CommandSwitchCellanim>(
                                    n, i
                                ));
                            }
                        }
                    }

                    ImGui::EndPopup();
                }

                ImGui::SetItemTooltip(
                    "Path: %s\nCellanim: %s\n\nRight-click to select the cellanim.",
                    session.resourcePath.c_str(),
                    session.getCurrentCellAnim().name.c_str()
                );

                if (!sessionOpen) {
                    sessionManager.setSessionClosingIndex(n);

                    if (session.modified)
                        OPEN_GLOBAL_POPUP("###CloseModifiedSession");
                    else {
                        sessionManager.RemoveSession(n);
                    }
                }

                ImGui::PopID();
            }
            ImGui::EndTabBar();
        }

        ImGui::EndMenuBar();
    }
}

void Toast::Update() {
    syncToUpdateRate();

    glfwMakeContextCurrent(mGlfwWindowHndl);

    glfwPollEvents();

    ImGui::SetCurrentContext(mGuiContext);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Begin main window.
    constexpr ImGuiDockNodeFlags dockspaceFlags =
        ImGuiDockNodeFlags_PassthruCentralNode;
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
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

    ImGui::Begin(WINDOW_TITLE "###HeadWindow", nullptr, windowFlags);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    // Submit the dockspace.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        ImGui::DockSpace(ImGui::GetID("HeadDockspace"), { -1.f, -1.f }, dockspaceFlags);

    Shortcuts::Process();

    Menubar();

    if (SessionManager::getInstance().getSessionAvaliable()) {
        PlayerManager::getInstance().Update();

        mWindowCanvas.Update();
        mWindowHybridList.Update();
        mWindowInspector.Update();
        mWindowTimeline.Update();
        mWindowSpritesheet.Update();
    }

    mWindowConfig.Update();
    mWindowAbout.Update();

    mWindowDemo.Update();

    Popups::Update();

    // End main window.
    ImGui::End();

    AsyncTaskManager::getInstance().UpdateTasks();

    MainThreadTaskManager::getInstance().Update();
}

void Toast::Draw() {
    ImGui::Render();

    glfwMakeContextCurrent(mGlfwWindowHndl);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(mGlfwWindowHndl, &framebufferWidth, &framebufferHeight);

    glViewport(0, 0, framebufferWidth, framebufferHeight);

    const auto& windowClearColor = ThemeManager::getInstance().getWindowClearColor();
    glClearColor(
        windowClearColor[0], windowClearColor[1],
        windowClearColor[2], windowClearColor[3]
    );

    glClear(GL_COLOR_BUFFER_BIT);

    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr) {
        throw std::runtime_error("Toast::Draw: ImGui::GetDrawData returned NULL!");
    }

    ImGui_ImplOpenGL3_RenderDrawData(drawData);

    // Update and draw platform windows.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        // Restore GLFW context.
        glfwMakeContextCurrent(mGlfwWindowHndl);
    }

    glfwSwapBuffers(mGlfwWindowHndl);
}
