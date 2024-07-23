#include "App.hpp"

#include "font/SegoeUI.h"
#include "font/FontAwesome.h"

#include <imgui_internal.h>

#include <chrono>

#include <iostream>

#include <string>
#include <sstream>

#include "common.hpp"

#include <tinyfiledialogs.h>

//#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "SessionManager.hpp"
#include "ConfigManager.hpp"
#include "PlayerManager.hpp"

#include "command/CommandSwitchCellanim.hpp"

#include "task/AsyncTaskManager.hpp"
#include "task/AsyncTask_ExportSessionTask.hpp"
#include "task/AsyncTask_PushSessionTask.hpp"

#include "MtCommandManager.hpp"

#include "_binary/images/toastIcon.png.h"

App* gAppPtr{ nullptr };

#define MAX_

#if defined(__APPLE__)
    #define SCS_EXIT "Cmd+Q"
    #define SCST_CTRL "Cmd"
    #define SCT_CTRL ImGui::GetIO().KeySuper
#else
    #define SCS_EXIT "Alt+F4"
    #define SCST_CTRL "Ctrl"
    #define SCT_CTRL ImGui::GetIO().KeyCtrl
#endif

#define SCS_LAUNCH_OPEN_SZS_DIALOG SCST_CTRL "+O"
#define SC_LAUNCH_OPEN_SZS_DIALOG \
            SCT_CTRL && \
            !ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_O)

#define SCS_LAUNCH_OPEN_TRADITIONAL_DIALOG SCST_CTRL "+Shift+O"
#define SC_LAUNCH_OPEN_TRADITIONAL_DIALOG \
            SCT_CTRL && \
            ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_O)

#define SCS_SAVE_CURRENT_SESSION_SZS SCST_CTRL "+S"
#define SC_SAVE_CURRENT_SESSION_SZS \
            SCT_CTRL && \
            !ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_S)

#define SCS_LAUNCH_SAVE_AS_SZS_DIALOG SCST_CTRL "+Shift+S"
#define SC_LAUNCH_SAVE_AS_SZS_DIALOG \
            SCT_CTRL && \
            ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_S)

#define SCS_UNDO SCST_CTRL "+Z"
#define SC_UNDO \
            SCT_CTRL && \
            !ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_Z)

#define SCS_REDO SCST_CTRL "+Shift+Z"
#define SC_REDO \
            SCT_CTRL && \
            ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_Z)

#pragma region Actions

void A_LD_CreateCompressedArcSession() {
    const char* filterPatterns[] = { "*.szs" };
    char* openFileDialog = tinyfd_openFileDialog(
        "Select a cellanim file",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Compressed Arc File (.szs)",
        false
    );

    if (!openFileDialog)
        return;

    AsyncTaskManager::getInstance().StartTask<PushSessionTask>(
        std::string(openFileDialog)
    );
}

void A_LD_CreateTraditionalSession() {
    char* pathList[3];

    const char* filterPatterns[] = { "*.brcad", "*.png", "*.h" };

    char* brcadDialog = tinyfd_openFileDialog(
        "Select the BRCAD file",
        nullptr,
        1, filterPatterns + 0,
        "Binary Revolution CellAnim Data file (.brcad)",
        false
    );
    if (!brcadDialog)
        return;

    pathList[0] = new char[strlen(brcadDialog) + 1];
    strcpy(pathList[0], brcadDialog);

    char* imageDialog = tinyfd_openFileDialog(
        "Select the image file",
        nullptr,
        1, filterPatterns + 1,
        "Image file (.png)",
        false
    );
    if (!imageDialog)
        return;

    pathList[1] = new char[strlen(imageDialog) + 1];
    strcpy(pathList[1], imageDialog);

    char* headerDialog = tinyfd_openFileDialog(
        "Select the header file",
        nullptr,
        1, filterPatterns + 2,
        "Header file (.h)",
        false
    );
    if (!headerDialog)
        return;

    pathList[2] = new char[strlen(headerDialog) + 1];
    strcpy(pathList[2], headerDialog);

    GET_SESSION_MANAGER;

    int32_t result = sessionManager.PushSessionTraditional((const char **)pathList);
    if (result < 0) {
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
        ImGui::OpenPopup("###SessionOpenErr");
        ImGui::PopID();
    }
    else {
        sessionManager.currentSession = result;
        sessionManager.SessionChanged();
    }
}

void A_LD_SaveCurrentSessionAsSzs() {
    GET_SESSION_MANAGER;
    GET_ASYNC_TASK_MANAGER;

    if (
        !sessionManager.getCurrentSession() ||
        asyncTaskManager.HasTaskOfType<ExportSessionTask>()
    )
        return;

    const char* filterPatterns[] = { "*.szs" };
    char* saveFileDialog = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Compressed Arc File (.szs)"
    );

    if (saveFileDialog)
        asyncTaskManager.StartTask<ExportSessionTask>(
            sessionManager.getCurrentSession(),
            std::string(saveFileDialog)
        );
}

void A_SaveCurrentSessionSzs() {
    GET_SESSION_MANAGER;
    GET_ASYNC_TASK_MANAGER;

    if (
        !sessionManager.getCurrentSession() ||
        sessionManager.getCurrentSession()->traditionalMethod ||

        asyncTaskManager.HasTaskOfType<ExportSessionTask>()
    )
        return;

    asyncTaskManager.StartTask<ExportSessionTask>(
        sessionManager.getCurrentSession(),
        sessionManager.getCurrentSession()->mainPath
    );
}

#pragma endregion

void App::AttemptExit(bool force) {
    GET_SESSION_MANAGER;

    if (!force)
        for (uint32_t i = 0; i < sessionManager.sessionList.size(); i++) {
            if (sessionManager.sessionList.at(i).modified) {
                this->dialog_warnExitWithUnsavedChanges = true;
                return; // Cancel
            }
        }

    GET_CONFIG_MANAGER;

    ConfigManager::Config config = configManager.getConfig();

    glfwGetWindowSize(window, &config.lastWindowWidth, &config.lastWindowHeight);

    configManager.setConfig(config);
    configManager.SaveConfig();

    this->running = false;
}

App::App() {
    gAppPtr = this;

    glfwSetErrorCallback([](int error_code, const char* description) {
        std::cerr << "GLFW Error (" << error_code << "): " << description << '\n';
    });
    assert(glfwInit());

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
    configManager.LoadConfig();

    this->windowThreadID = std::this_thread::get_id();

    this->window = glfwCreateWindow(
        configManager.getConfig().lastWindowWidth,
        configManager.getConfig().lastWindowHeight,
        WINDOW_TITLE,
        nullptr, nullptr
    );
    assert(this->window);

    glfwSetWindowCloseCallback(this->window, [](GLFWwindow* window) {
        extern App* gAppPtr;
        gAppPtr->AttemptExit();
    });

    glfwMakeContextCurrent(this->window);
    //glfwSwapInterval(1); // Enable vsync

    // Icon
    GLFWimage windowIcon;
    windowIcon.pixels = stbi_load_from_memory(toastIcon_png, toastIcon_png_size, &windowIcon.width, &windowIcon.height, nullptr, 4);

    glfwSetWindowIcon(window, 1, &windowIcon); 
    stbi_image_free(windowIcon.pixels);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    GET_IMGUI_IO;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    GET_APP_STATE;

    appState.UpdateTheme();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.f;
    }

    style.WindowPadding.x = 10;
    style.WindowPadding.y = 10;

    style.FrameRounding = 5;
    style.WindowRounding = 5;
    style.ChildRounding = 5;
    style.GrabRounding = 5;
    style.PopupRounding = 5;

    style.FramePadding.x = 5;
    style.FramePadding.y = 3;

    style.ItemSpacing.x = 9;
    style.ItemSpacing.y = 4;

    style.WindowTitleAlign.x = 0.5f;
    style.WindowTitleAlign.y = 0.5f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    this->SetupFonts();

    this->windowCanvas = new WindowCanvas;
    this->windowHybridList = new WindowHybridList;
    this->windowInspector = new WindowInspector;
    this->windowTimeline = new WindowTimeline;
    this->windowSpritesheet = new WindowSpritesheet;

    this->windowConfig = new WindowConfig;
    this->windowAbout = new WindowAbout;
}

App::~App() {
    glfwMakeContextCurrent(this->window);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    SessionManager::getInstance().FreeAllSessions();

    Common::deleteIfNotNullptr(AppState::getInstance().globalAnimatable);

    Common::deleteIfNotNullptr(this->windowCanvas);
    Common::deleteIfNotNullptr(this->windowHybridList);
    Common::deleteIfNotNullptr(this->windowInspector);
    Common::deleteIfNotNullptr(this->windowTimeline);
    Common::deleteIfNotNullptr(this->windowSpritesheet);
    Common::deleteIfNotNullptr(this->windowConfig);
    Common::deleteIfNotNullptr(this->windowAbout);
}

void App::SetupFonts() {
    GET_IMGUI_IO;
    GET_APP_STATE;

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fonts.normal = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 18.0f, &fontConfig);
    }

    {
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig fontConfig;
        fontConfig.MergeMode = true;
        fontConfig.PixelSnapH = true;

        appState.fonts.icon = io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_FA, 15.0f, &fontConfig, icons_ranges);
    }

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fonts.large = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 24.0f, &fontConfig);
    }

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fonts.giant = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 52.0f, &fontConfig);
    }
}

void App::Menubar() {
    GET_APP_STATE;
    GET_SESSION_MANAGER;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(WINDOW_TITLE)) {
            if (ImGui::MenuItem((char*)ICON_FA_SHARE_FROM_SQUARE " About " WINDOW_TITLE, "", nullptr)) {
                this->windowAbout->open = true;
                ImGui::SetWindowFocus("About");
            }

            ImGui::Separator();

            if (ImGui::MenuItem((char*)ICON_FA_WRENCH " Config", "", nullptr)) {
                this->windowConfig->open = true;
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
                A_LD_CreateCompressedArcSession();

            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_IMPORT " Open (seperated)...",
                SCS_LAUNCH_OPEN_TRADITIONAL_DIALOG,
                nullptr
            ))
                A_LD_CreateTraditionalSession();

            ImGui::Separator();

            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_EXPORT " Save (szs)", SCS_SAVE_CURRENT_SESSION_SZS, false,
                    !!sessionManager.getCurrentSession() &&
                    !sessionManager.getCurrentSession()->traditionalMethod
            ))
                A_SaveCurrentSessionSzs();

            if (ImGui::MenuItem(
                (char*)ICON_FA_FILE_EXPORT " Save as (szs)...", SCS_LAUNCH_SAVE_AS_SZS_DIALOG, false,
                !!sessionManager.getCurrentSession()
            ))
                A_LD_SaveCurrentSessionAsSzs();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem(
                (char*)ICON_FA_ARROW_ROTATE_LEFT " Undo", SCS_UNDO, false,
                    !!sessionManager.getCurrentSession() &&
                    sessionManager.getCurrentSession()->canUndo()
            ))
                sessionManager.getCurrentSession()->undo();

            if (ImGui::MenuItem(
                (char*)ICON_FA_ARROW_ROTATE_RIGHT " Redo", SCS_REDO, false,
                    !!sessionManager.getCurrentSession() &&
                    sessionManager.getCurrentSession()->canRedo()
            ))
                sessionManager.getCurrentSession()->redo();

            ImGui::EndMenu();
        }

        const bool sessionAvaliable = sessionManager.currentSession >= 0;

        if (ImGui::BeginMenu("Cellanim", sessionAvaliable)) {
            if (ImGui::BeginMenu("Select")) {
                for (uint16_t i = 0; i < sessionManager.getCurrentSession()->cellanims.size(); i++) {
                    const std::string& str = sessionManager.getCurrentSession()->cellanims.at(i).name;

                    std::stringstream fmtStream;
                    fmtStream << std::to_string(i+1) << ". " << str.substr(0, str.size() - 6);

                    if (ImGui::MenuItem(
                        fmtStream.str().c_str(), nullptr,
                        sessionManager.getCurrentSession()->currentCellanim == i
                    )) {
                        if (sessionManager.getCurrentSession()->currentCellanim != i) {
                            sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandSwitchCellanim>(
                            CommandSwitchCellanim(
                                sessionManager.currentSession, i
                            )));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Optimize ..");

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

            ImGui::Separator();

            ImGui::Text("Make arrangent unique (duplicate)");

            ImGui::Separator();

            ImGui::MenuItem("Interpolate ..");

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

            ImGui::MenuItem("Delete selected key");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Arrangement", sessionAvaliable)) {
            ImGui::Text(
                "Current arrangement (no. %u)",
                appState.globalAnimatable->getCurrentKey()->arrangementIndex
            );

            ImGui::Separator();

            ImGui::MenuItem("Transform as whole ..");

            ImGui::Separator();

            if (ImGui::BeginMenu("Region")) {
                ImGui::MenuItem("Pad (Expand/Contract) ..");

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Part",
            sessionAvaliable && appState.selectedPart >= 0
        )) {
            ImGui::Text(
                "Selected part (no. %u)",
                appState.selectedPart
            );

            ImGui::Separator();

            ImGui::MenuItem("Visible", nullptr, true);
            ImGui::MenuItem("Locked");

            ImGui::Separator();

            if (ImGui::BeginMenu("Region")) {
                ImGui::MenuItem("Pad (Expand/Contract) ..");

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Set name (identifier) ..");

            ImGui::Separator();

            ImGui::MenuItem("Delete");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            if (ImGui::MenuItem("Canvas", "", !!this->windowCanvas)) {
                if (this->windowCanvas)
                    Common::deleteIfNotNullptr(this->windowCanvas);
                else
                    this->windowCanvas = new WindowCanvas;
            }

            if (ImGui::MenuItem("Animation- / Arrangement List", "", !!this->windowHybridList)) {
                if (this->windowHybridList)
                    Common::deleteIfNotNullptr(this->windowHybridList);
                else
                    this->windowHybridList = new WindowHybridList;
            }

            if (ImGui::MenuItem("Inspector", "", !!this->windowInspector)) {
                if (this->windowInspector)
                    Common::deleteIfNotNullptr(this->windowInspector);
                else
                    this->windowInspector = new WindowInspector;
            }

            if (ImGui::MenuItem("Spritesheet", "", !!this->windowSpritesheet)) {
                if (this->windowSpritesheet)
                    Common::deleteIfNotNullptr(this->windowSpritesheet);
                else
                    this->windowSpritesheet = new WindowSpritesheet;
            }

            if (ImGui::MenuItem("Timeline", "", !!this->windowTimeline)) {
                if (this->windowTimeline)
                    Common::deleteIfNotNullptr(this->windowTimeline);
                else
                    this->windowTimeline = new WindowTimeline;
            }

            ImGui::Separator();

            ImGui::MenuItem("Dear ImGui demo window", "", &appState.enableDemoWindow);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMenuBar()) {
        ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll;

        GET_SESSION_MANAGER;

        if (ImGui::BeginTabBar("FileTabBar", tabBarFlags)) {
            for (int n = 0; n < sessionManager.sessionList.size(); n++) {
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
                        for (uint16_t i = 0; i < sessionManager.sessionList.at(n).cellanims.size(); i++) {
                            const std::string& str = sessionManager.sessionList.at(n).cellanims.at(i).name;

                            std::stringstream fmtStream;
                            fmtStream << std::to_string(i+1) << ". " << str.substr(0, str.size() - 6);

                            if (ImGui::MenuItem(
                                fmtStream.str().c_str(), nullptr,
                                sessionManager.sessionList.at(n).currentCellanim == i
                            )) {
                                ImGui::CloseCurrentPopup();

                                if (sessionManager.sessionList.at(n).currentCellanim != i) {
                                    sessionManager.currentSession = n;

                                    sessionManager.sessionList.at(n).executeCommand(std::make_shared<CommandSwitchCellanim>(
                                    CommandSwitchCellanim(
                                        n, i
                                    )));
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

                    if (sessionManager.sessionList.at(n).modified) {
                        ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                        ImGui::OpenPopup("###ConfirmFreeModifiedSession");
                        ImGui::PopID();
                    }
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

void App::UpdatePopups() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::PushOverrideID(AppState::getInstance().globalPopupID);

    // ###SessionOpenErr
    // ###SessionOutErr
    {
        SessionManager::SessionError errorCode = SessionManager::getInstance().lastSessionError;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal((
            "There was an error opening the session. (" +
            std::to_string(errorCode) +
            ")###SessionOpenErr"
        ).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            std::string errorMessage;
            switch (errorCode) {
                case SessionManager::SessionOpenError_FailOpenArchive:
                    errorMessage = "The archive file could not be opened.";
                    break;
                case SessionManager::SessionOpenError_FailFindTPL:
                    errorMessage = "The archive does not contain the cellanim.tpl file.";
                    break;
                case SessionManager::SessionOpenError_RootDirNotFound:
                    errorMessage = "The archive does not contain a root directory.";
                    break;
                case SessionManager::SessionOpenError_NoBXCADsFound:
                    errorMessage = "The archive does not contain any brcad/bccad files.";
                    break;
                case SessionManager::SessionOpenError_FailOpenBXCAD:
                    errorMessage = "The brcad/bccad file could not be opened.";
                    break;
                case SessionManager::SessionOpenError_FailOpenPNG:
                    errorMessage = "The image file (.png) could not be opened.";
                    break;
                case SessionManager::SessionOpenError_FailOpenHFile:
                    errorMessage = "The header file (.h) could not be opened.";
                    break;
                case SessionManager::SessionOpenError_SessionsFull:
                    errorMessage = "The maximum amount of sessions are already open.";
                    break;
                default:
                    errorMessage = "An unknown error has occurred (" + std::to_string(errorCode) + ").";
                    break;
            }

            ImGui::TextUnformatted(errorMessage.c_str());

            ImGui::Dummy({0, 5});

            if (ImGui::Button("Alright", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
        

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal((
            "There was an error exporting the session. (" +
            std::to_string(errorCode) +
            ")###SessionOutErr"
        ).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            std::string errorMessage;
            switch (errorCode) {
                case SessionManager::SessionOutError_FailOpenFile:
                    errorMessage = "The destination file could not be opened for reading.";
                    break;
                case SessionManager::SessionOutError_ZlibError:
                    errorMessage = "Zlib raised an error while compressing the file.";
                    break;
                case SessionManager::SessionOutError_FailTPLTextureExport:
                    errorMessage = "There was an error exporting a texture.";
                    break;
                default:
                    errorMessage = "An unknown error has occurred (" + std::to_string(errorCode) + ").";
                    break;
            }

            ImGui::TextUnformatted(errorMessage.c_str());

            ImGui::Dummy({0, 5});

            if (ImGui::Button("Alright", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
        
        ImGui::PopStyleVar();
    }

    // ###DialogWaitForModifiedPNG
    {
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        if (ImGui::BeginPopupModal("Modifying texture via image editor###DialogWaitForModifiedPNG", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Once the modified texture is ready, select the \"Ready\" option.\n To cancel, select \"Nevermind\".");

            ImGui::Dummy({ 0, 15 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            const char* formatList[] = {
                "RGBA32 (full-depth color)",
                "RGB5A3 (variable color depth)",
            };
            static int selectedFormatIndex{ 0 };
            ImGui::Combo("Image Format", &selectedFormatIndex, formatList, ARRAY_LENGTH(formatList));

            ImGui::Dummy({ 0, 5 });

            if (ImGui::Button("Ready", ImVec2(120, 0))) {
                GET_SESSION_MANAGER;

                std::shared_ptr<Common::Image> newImage =
                    std::make_shared<Common::Image>(Common::Image());
                newImage->LoadFromFile(ConfigManager::getInstance().getConfig().textureEditPath.c_str());

                if (newImage->texture) {
                    auto& cellanimSheet = sessionManager.getCurrentSession()->getCellanimSheet();

                    bool diffSize =
                        newImage->width  != cellanimSheet->width ||
                        newImage->height != cellanimSheet->height;

                    cellanimSheet = newImage;

                    switch (selectedFormatIndex) {
                        case 0:
                            cellanimSheet->tplOutFormat = TPL::TPL_IMAGE_FORMAT_RGBA32;
                            break;
                        
                        case 1:
                            cellanimSheet->tplOutFormat = TPL::TPL_IMAGE_FORMAT_RGB5A3;
                            break;
                        
                        default:
                            break;
                    }

                    ImGui::CloseCurrentPopup();

                    if (diffSize) {
                        ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
                        ImGui::OpenPopup("###DialogModifiedPNGSizeDiff");
                        ImGui::PopID();
                    }

                    sessionManager.SessionChanged();
                    sessionManager.getCurrentSessionModified() = true;
                }
                else
                    ImGui::CloseCurrentPopup();
            } ImGui::SetItemDefaultFocus();

            ImGui::SameLine();

            if (ImGui::Button("Nevermind", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    // ###DialogPNGExportFailed
    {
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        if (ImGui::BeginPopupModal("There was an error exporting the texture.###DialogPNGExportFailed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("The texture could not be written to the PNG file.\nPlease check the path set in the config.");

            ImGui::Dummy({0, 5});

            if (ImGui::Button("Alright", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    // ###DialogModifiedPNGSizeDiff
    {
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        if (ImGui::BeginPopupModal("Texture size mismatch###DialogModifiedPNGSizeDiff", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("The new texture's size is different from the existing texture.\nPlease select the wanted behavior:");

            ImGui::Dummy({ 0, 15 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            ImGui::PopStyleVar();

            if (ImGui::Button("No region scaling")) {
                SessionManager::Session* currentSession = SessionManager::getInstance().getCurrentSession();

                currentSession->getCellanimObject()->textureW = currentSession->getCellanimSheet()->width;
                currentSession->getCellanimObject()->textureH = currentSession->getCellanimSheet()->height;

                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                ImGui::SetTooltip("Select this option for texture extensions.");

            ImGui::SameLine();

            if (ImGui::Button("Apply region scaling")) {
                SessionManager::Session* currentSession = SessionManager::getInstance().getCurrentSession();

                float scaleX =
                    static_cast<float>(currentSession->getCellanimSheet()->width) /
                    currentSession->getCellanimObject()->textureW;
                float scaleY =
                    static_cast<float>(currentSession->getCellanimSheet()->height) /
                    currentSession->getCellanimObject()->textureH;

                for (auto& arrangement : currentSession->getCellanimObject()->arrangements)
                    for (auto& part : arrangement.parts) {
                        part.regionX *= scaleX;
                        part.regionW *= scaleX;
                        part.regionY *= scaleY;
                        part.regionH *= scaleY;

                        part.scaleX /= scaleX;
                        part.scaleY /= scaleY;
                    }

                currentSession->getCellanimObject()->textureW = currentSession->getCellanimSheet()->width;
                currentSession->getCellanimObject()->textureH = currentSession->getCellanimSheet()->height;

                ImGui::CloseCurrentPopup();
            } ImGui::SetItemDefaultFocus();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                ImGui::SetTooltip("Select this option for texture upscales.");

            ImGui::EndPopup();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 }); // sigh
        }
        ImGui::PopStyleVar();
    }

    // ###ConfirmFreeModifiedSession
    {
        GET_SESSION_MANAGER;

        std::string popupTitle =
            sessionManager.sessionClosing < sessionManager.sessionList.size() ?
            sessionManager.sessionList.at(sessionManager.sessionClosing).mainPath :
            "Confirm";

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        if (ImGui::BeginPopupModal((
            popupTitle +
            "###ConfirmFreeModifiedSession"
        ).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            // -50 to account for window padding
            ImGui::Dummy({ ImGui::CalcTextSize(popupTitle.c_str()).x - 40, 0 });

            ImGui::Text("Are you sure you want to close this session?\nYour changes will be lost.");

            ImGui::Dummy({ 0, 15 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();

            if (ImGui::Button("Close without saving")) {
                sessionManager.FreeSessionIndex(sessionManager.sessionClosing);
                sessionManager.SessionChanged();

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    // ###AttemptExitWhileUnsavedChanges
    {
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        if (ImGui::BeginPopupModal("Exit with unsaved changes###AttemptExitWhileUnsavedChanges", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("There are still unsaved changes in one or more sessions.\nAre you sure you want to close the app?");

            ImGui::Dummy({ 0, 15 });
            ImGui::Separator();
            ImGui::Dummy({ 0, 5 });

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();

            if (ImGui::Button("Exit without saving")) {
                ImGui::CloseCurrentPopup();

                this->AttemptExit(true);
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    ImGui::PopID();
}

void App::HandleShortcuts() {
    if (SC_LAUNCH_OPEN_SZS_DIALOG)
        A_LD_CreateCompressedArcSession();
    if (SC_LAUNCH_OPEN_TRADITIONAL_DIALOG)
        A_LD_CreateTraditionalSession();
    if (SC_SAVE_CURRENT_SESSION_SZS)
        A_SaveCurrentSessionSzs();
    if (SC_LAUNCH_SAVE_AS_SZS_DIALOG)
        A_LD_SaveCurrentSessionAsSzs();

    GET_SESSION_MANAGER;

    if (sessionManager.getCurrentSession()) {
        if (SC_UNDO)
            sessionManager.getCurrentSession()->undo();
        if (SC_REDO)
            sessionManager.getCurrentSession()->redo();
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

    this->BeginMainWindow();
    this->Menubar();

    this->HandleShortcuts();

    if (appState.enableDemoWindow)
        ImGui::ShowDemoWindow(&appState.enableDemoWindow);

    if (!!SessionManager::getInstance().getCurrentSession()) {
        PlayerManager::getInstance().Update();

        // Windows
        if (this->windowCanvas)
            this->windowCanvas->Update();
        if (this->windowHybridList)
            this->windowHybridList->Update();
        if (this->windowInspector)
            this->windowInspector->Update();
        if (this->windowTimeline)
            this->windowTimeline->Update();
        if (this->windowSpritesheet)
            this->windowSpritesheet->Update();
    }
    
    if (this->windowConfig->open)
        this->windowConfig->Update();
    if (this->windowAbout->open)
        this->windowAbout->Update();

    if (this->dialog_warnExitWithUnsavedChanges) {
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
        ImGui::OpenPopup("###AttemptExitWhileUnsavedChanges");
        ImGui::PopID();

        this->dialog_warnExitWithUnsavedChanges = false;
    }

    AsyncTaskManager::getInstance().UpdateTasks();

    MtCommandManager::getInstance().Update();

    this->UpdatePopups();

    // TODO: Implement timeline auto-scroll
    // static bool autoScrollTimeline{ false };

    // End main window
    ImGui::End();

    // Render
    ImGui::Render();

    int fbSize[2];
    glfwGetFramebufferSize(window, fbSize, fbSize + 1);
    glViewport(0, 0, fbSize[0], fbSize[1]);

    const ImVec4 clearColor = appState.getWindowClearColor();
    glClearColor(
        clearColor.x * clearColor.w,
        clearColor.y * clearColor.w,
        clearColor.z * clearColor.w,
        clearColor.w
    );
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        glfwMakeContextCurrent(backupCurrentContext);
    }

    glfwSwapBuffers(window);
}