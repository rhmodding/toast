#include "App.hpp"

#include "font/SegoeUI.h"
#include "font/FontAwesome.h"

#include <chrono>

#include <thread>

#include <iostream>
#include <vector>

#include <string>
#include <sstream>

#include "archive/U8.hpp"

#include "imgui_internal.h"

#include "SessionManager.hpp"

#include "ConfigManager.hpp"

#include <tinyfiledialogs.h>

//#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "_binary/images/toastIcon.png.h"

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

    GET_SESSION_MANAGER;

    int32_t result = sessionManager.PushSessionFromCompressedArc(openFileDialog);
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

    if (!sessionManager.getCurrentSession())
        return;

    const char* filterPatterns[] = { "*.szs" };
    char* saveFileDialog = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Compressed Arc File (.szs)"
    );

    if (saveFileDialog) {
        int32_t result = sessionManager.ExportSessionCompressedArc(
            sessionManager.getCurrentSession(), saveFileDialog
        );
        if (result < 0) {
            ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
            ImGui::OpenPopup("###SessionOutErr");
            ImGui::PopID();
        }
        else {
            sessionManager.getCurrentSession()->mainPath = saveFileDialog;
            sessionManager.getCurrentSession()->traditionalMethod = false;
        }
    }
}

void A_SaveCurrentSessionSzs() {
    GET_SESSION_MANAGER;

    if (
        !sessionManager.getCurrentSession() ||
        sessionManager.getCurrentSession()->traditionalMethod
    )
        return;

    int32_t result = sessionManager.ExportSessionCompressedArc(
        sessionManager.getCurrentSession(),
        sessionManager.getCurrentSession()->mainPath.c_str()
    );

    if (result < 0) {
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID);
        ImGui::OpenPopup("###SessionOutErr");
        ImGui::PopID();
    }
}

#pragma endregion

App* gAppPtr{ nullptr };

void App::AttemptExit() {
    GET_SESSION_MANAGER;

    if (!this->exitWithUnsavedChanges)
        for (uint32_t i = 0; i < sessionManager.sessionList.size(); i++) {
            if (sessionManager.sessionList.at(i).modified) {
                this->dialog_warnExitWithUnsavedChanges = true;

                // Cancel close
                return;
            }
        }

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    GET_CONFIG_MANAGER;

    configManager.config.lastWindowWidth = windowWidth;
    configManager.config.lastWindowHeight = windowHeight;

    configManager.SaveConfig();

    this->stopMainLoop = true;
}

App::App() {
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

    this->window = glfwCreateWindow(
        configManager.config.lastWindowWidth,
        configManager.config.lastWindowHeight,
        WINDOW_TITLE,
        nullptr, nullptr
    );
    assert(this->window);

    gAppPtr = this;
    glfwSetWindowCloseCallback(this->window, [](GLFWwindow* window) {
        extern App* gAppPtr;
        gAppPtr->AttemptExit();
    });

    glfwMakeContextCurrent(this->window);
    glfwSwapInterval(1); // Enable vsync

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
    appState.UpdateUpdateRate();

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

    style.FramePadding.x = 5;
    style.FramePadding.y = 3;

    style.ItemSpacing.x = 9;
    style.ItemSpacing.y = 4;

    style.PopupRounding = 5;

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
        appState.fontNormal = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 18.0f, &fontConfig);
    }

    {
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig fontConfig;
        fontConfig.MergeMode = true;
        fontConfig.PixelSnapH = true;

        appState.fontIcon = io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_FA, 15.0f, &fontConfig, icons_ranges);
    }

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fontLarge = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 24.0f, &fontConfig);
    }

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fontGiant = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 52.0f, &fontConfig);
    }
}

void App::Menubar() {
    GET_APP_STATE;

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
            GET_SESSION_MANAGER;

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

            ImGui::Separator();

            // TODO: add Apple shortcut
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

                bool sessionOpen = sessionManager.sessionList.at(n).open;
                if (
                    sessionManager.sessionList.at(n).open &&
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
                            std::string* str = &sessionManager.sessionList.at(n).cellNames.at(i);

                            std::stringstream fmtStream;
                            fmtStream << std::to_string(i) << ". " << str->substr(0, str->size() - 6);

                            if (ImGui::MenuItem(fmtStream.str().c_str(), nullptr, sessionManager.sessionList.at(n).cellIndex == i)) {
                                ImGui::CloseCurrentPopup();
                                sessionManager.sessionList.at(n).cellIndex = i;
                                sessionManager.SessionChanged();
                            }
                        }
                            
                        ImGui::EndPopup();
                    }
                }

                if (sessionManager.sessionList.at(n).open) {
                    std::string* cellName = &sessionManager.sessionList.at(n).cellNames.at(
                        sessionManager.sessionList.at(n).cellIndex
                    );

                    ImGui::SetItemTooltip(
                        "Path: %s\nCellanim: %s\n\nRight-click to select the cellanim.",
                        sessionManager.sessionList.at(n).mainPath.c_str(),
                        cellName->substr(0, cellName->size() - 6).c_str()
                    );
                }

                if (!sessionOpen && sessionManager.sessionList.at(n).open) {
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

            if (ImGui::Button("Ready", ImVec2(120, 0))) {
                GET_SESSION_MANAGER;

                Common::Image* newImage = new Common::Image();
                newImage->LoadFromFile(ConfigManager::getInstance().config.textureEditPath.c_str());

                if (newImage->texture) {
                    SessionManager::getInstance().getCurrentSessionModified() |= true;

                    sessionManager.getCurrentSession()->getCellanimSheet()->FreeTexture();
                    delete sessionManager.getCurrentSession()->getCellanimSheet();
                    sessionManager.getCurrentSession()->getCellanimSheet() = newImage;
                    
                    sessionManager.SessionChanged();
                }

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

                this->exitWithUnsavedChanges = true;
                this->AttemptExit();
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
}

void App::Update() {
    GET_APP_STATE;

    // Update rate
    {
        static std::chrono::system_clock::time_point a{ std::chrono::system_clock::now() };
        static std::chrono::system_clock::time_point b{ std::chrono::system_clock::now() };

        a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> workTime = a - b;

        if (workTime.count() < appState.updateRate) {
            std::chrono::duration<double, std::milli> deltaMs(appState.updateRate - workTime.count());
            auto deltaMsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(deltaMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(deltaMsDuration.count()));
        }

        b = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> sleep_time = b - a;
    }

    static ImGuiIO& io{ ImGui::GetIO() };

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
        appState.playerState.Update();

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

    this->UpdatePopups();

    // TODO: Implement timeline auto-scroll
    // static bool autoScrollTimeline{ false };

    // End main window
    ImGui::End();

    // Render
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(
        appState.windowClearColor.x * appState.windowClearColor.w,
        appState.windowClearColor.y * appState.windowClearColor.w,
        appState.windowClearColor.z * appState.windowClearColor.w,
        appState.windowClearColor.w
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