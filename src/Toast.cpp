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
#include "cellanim/CellAnimRenderer.hpp"

#include "manager/AppState.hpp"

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/PlayerManager.hpp"
#include "manager/ThemeManager.hpp"
#include "manager/MainThreadTaskManager.hpp"
#include "manager/PromptPopupManager.hpp"

#include "font/FontAwesome.h"

#include "command/CommandSwitchCellanim.hpp"
#include "command/CommandDeleteArrangement.hpp"
#include "command/CommandModifyArrangementPart.hpp"
#include "command/CommandDeleteArrangementPart.hpp"
#include "command/CommandModifyAnimationKey.hpp"
#include "command/CommandDeleteAnimationKey.hpp"
#include "command/CommandMoveAnimationKey.hpp"
#include "command/CommandInsertAnimationKey.hpp"

#include "manager/AsyncTaskManager.hpp"

#include "task/AsyncTaskPushSession.hpp"

#include "App/Actions.hpp"
#include "App/Shortcuts.hpp"

#include "App/PopupHandler.hpp"
#include "App/popups/AllPopups.hpp"

// macOS doesn't support assigning a window icon
#if !defined(__APPLE__)
#include "stb/stb_image.h"

#include "BIN/image/toastIcon.png.h"
#endif // !defined(__APPLE__)

#include "BuildDate.hpp"

#define WINDOW_TITLE "toast"

Toast *Toast::gInstance { nullptr };
Toast *Toast::getInstance() {
    if (gInstance == nullptr) {
        throw std::runtime_error("Toast::getInstance: Instance of Toast does not exist!");
    }
    return gInstance;
}

static inline void syncToUpdateRate() {
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

Toast::Toast(int argc, const char **argv) {
    if (gInstance != nullptr) {
        throw std::runtime_error("Toast::Toast: Instance of Toast already exists!");
    }
    gInstance = this;

    mMainThreadId = std::this_thread::get_id();

    Logging::open("toast.log");

    glfwSetErrorCallback([](int code, const char* message) {
        Logging::error("GLFW threw an error (code {}): {}", code, message);
    });

    if (!glfwInit()) {
        throw std::runtime_error("Toast::Toast: Failed to init GLFW!");
    }

#ifdef NDEBUG
    Logging::info("Release: {}", gBuildDate);
#else
    Logging::info("Debug: {}", gBuildDate);
#endif

    MainThreadTaskManager::createSingleton();
    AsyncTaskManager::createSingleton();
    AppState::createSingleton();
    ThemeManager::createSingleton();
    ConfigManager::createSingleton();
    PlayerManager::createSingleton();
    SessionManager::createSingleton();
    PromptPopupManager::createSingleton();
    Popups::createSingletons();

#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glslVersion = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glslVersion = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
    // GL 3.0 + GLSL 130
    const char *glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only
#endif // defined(__APPLE__), defined(IMGUI_IMPL_OPENGL_ES2)

    ConfigManager& configManager = ConfigManager::getInstance();
    configManager.loadConfig();

    int windowWidth = configManager.getConfig().lastWindowWidth;
    int windowHeight = configManager.getConfig().lastWindowHeight;

    if (windowWidth < 1) {
        windowWidth = Config::DEFAULT_WINDOW_WIDTH;
    }
    if (windowHeight < 1) {
        windowHeight = Config::DEFAULT_WINDOW_HEIGHT;
    }

    mGlfwWindowHndl = glfwCreateWindow(
        windowWidth, windowHeight,
        WINDOW_TITLE,
        nullptr, nullptr
    );

    if (mGlfwWindowHndl == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Toast::Toast: glfwCreateWindow failed!");
    }

    glfwSetWindowCloseCallback(mGlfwWindowHndl, [](GLFWwindow *) {
        Toast::getInstance()->requestExit();
    });

    mIsWindowMaximized = configManager.getConfig().lastWindowMaximized;
    if (mIsWindowMaximized) {
        glfwMaximizeWindow(mGlfwWindowHndl);
    }

    glfwSetWindowMaximizeCallback(mGlfwWindowHndl, [](GLFWwindow *, int maximized) {
        Toast::getInstance()->mIsWindowMaximized = maximized != 0;
    });

    glfwMakeContextCurrent(mGlfwWindowHndl);
    glfwSwapInterval(1); // enable VSync

#if defined(USING_GLEW)
    if (GLenum glewInitResult = glewInit(); glewInitResult == GLEW_ERROR_NO_GLX_DISPLAY) {
        Logging::warn("[Toast::Toast] No GLX display; either running under Wayland or something is very wrong with your X11 server");
        tinyfd_messageBox(
            "Running under Wayland?",
            "Running toast under Wayland can lead to some stuff not working as it should. (blame glew/glfw)\n"
            "Consider using Xwayland by setting environment variable XDG_SESSION_TYPE=x11",
            "ok", 
            "warning", 
            1
        );
    } else if (glewInitResult != GLEW_OK) {
        std::string errorMesg ("Toast::Toast: Failed to init GLEW: ");
        errorMesg.append(reinterpret_cast<const char*>(glewGetErrorString(glewInitResult)));
        glfwDestroyWindow(mGlfwWindowHndl);
        glfwTerminate();

        throw std::runtime_error(errorMesg);
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
        ImGuiConfigFlags_NavEnableKeyboard | // enable Keyboard Controls,
        ImGuiConfigFlags_DockingEnable |     // enable Docking, and
        ImGuiConfigFlags_ViewportsEnable;    // enable Multi-Viewport / Platform Windows

    ImGui_ImplGlfw_InitForOpenGL(mGlfwWindowHndl, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    ThemeManager &themeManager = ThemeManager::getInstance();
    themeManager.initialize();
    themeManager.applyTheming();

    CellAnimRenderer::beginShader();

    mRootWindow = new WindowRoot;

    glfwSetDropCallback(mGlfwWindowHndl, [](GLFWwindow*, int pathCount, const char **path) {
        for (int i = 0; i < pathCount; i++) {
            AsyncTaskManager::getInstance().startTask<AsyncTaskPushSession>(
                std::string(path[i])
            );
        }
    });

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            AsyncTaskManager::getInstance().startTask<AsyncTaskPushSession>(
                std::string(argv[i])
            );
        }
    }

    Logging::info("[Toast::Toast] Finished initializing.");
}

Toast::~Toast() {
    glfwMakeContextCurrent(mGlfwWindowHndl);

    if (mRootWindow != nullptr) {
        delete mRootWindow;
    }

    SessionManager::destroySingleton();

    CellAnimRenderer::endShader();

    AppState::destroySingleton();
    ConfigManager::destroySingleton();
    PlayerManager::destroySingleton();
    ThemeManager::destroySingleton();
    AsyncTaskManager::destroySingleton();
    MainThreadTaskManager::destroySingleton();
    PromptPopupManager::destroySingleton();
    Popups::destroySingletons();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(mGuiContext);

    glfwDestroyWindow(mGlfwWindowHndl);
    glfwTerminate();

    Logging::info("[Toast::Toast] Finished deinitializing.");

    Logging::close();

    gInstance = nullptr;
}

void Toast::requestExit(bool force) {
    if (!force) {
        for (const auto &session : SessionManager::getInstance().getSessions()) {
            if (session.modified) {
                PromptPopupManager::getInstance().queue(
                    PromptPopupManager::createPrompt(
                        "Hang on!",
                        "There are still unsaved changes in one or more sessions.\n"
                        "Are you sure you want to close toast?"
                    )
                    .withResponses(
                        PromptPopup::RESPONSE_CANCEL | PromptPopup::RESPONSE_YES,
                        PromptPopup::RESPONSE_CANCEL
                    )
                    .withCallback([this](auto res, const std::string *) {
                        if (res == PromptPopup::RESPONSE_YES) {
                            requestExit(true);
                        }
                    })
                );
                return;
            }
        }
    }

    ConfigManager& configManager = ConfigManager::getInstance();

    Config config = configManager.getConfig();

    glfwGetWindowSize(mGlfwWindowHndl, &config.lastWindowWidth, &config.lastWindowHeight);
    config.lastWindowMaximized = mIsWindowMaximized;

    configManager.setConfig(config);
    configManager.saveConfig();

    mRunning = false;
}

void Toast::update() {
    mDrawnThisFrame = false;

    syncToUpdateRate();

    glfwMakeContextCurrent(mGlfwWindowHndl);

    glfwPollEvents();

    ImGui::SetCurrentContext(mGuiContext);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    mRootWindow->update();

    AsyncTaskManager::getInstance().update();

    MainThreadTaskManager::getInstance().update();
}

void Toast::draw() {
    if (mDrawnThisFrame) {
        Logging::warn("[Toast::draw] Already drawn this frame ..");
        return;
    }

    ImGui::Render();

    glfwMakeContextCurrent(mGlfwWindowHndl);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(mGlfwWindowHndl, &framebufferWidth, &framebufferHeight);

    glViewport(0, 0, framebufferWidth, framebufferHeight);

    const auto &windowClearColor = ThemeManager::getInstance().getWindowClearColor();
    glClearColor(
        windowClearColor[0], windowClearColor[1],
        windowClearColor[2], windowClearColor[3]
    );

    glClear(GL_COLOR_BUFFER_BIT);

    ImDrawData *drawData = ImGui::GetDrawData();
    if (drawData == nullptr) {
        throw std::runtime_error("Toast::draw: ImGui::GetDrawData returned NULL!");
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

    mDrawnThisFrame = true;
}
