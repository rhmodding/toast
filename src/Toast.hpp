#ifndef TOAST_HPP
#define TOAST_HPP

#include <imgui.h>

#include "glInclude.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <thread>

#include "window/BaseWindow.hpp"

#include "window/WindowCanvas.hpp"
#include "window/WindowHybridList.hpp"
#include "window/WindowInspector.hpp"
#include "window/WindowTimeline.hpp"
#include "window/WindowSpritesheet.hpp"
#include "window/WindowConfig.hpp"
#include "window/WindowAbout.hpp"
#include "window/WindowImguiDemo.hpp"

#include "Logging.hpp"

#include "common.hpp"

class Toast;

extern Toast* globlToast;

class Toast {
public:
    Toast(int argc, const char** argv);
    ~Toast();

public:
    void Update();
    void Draw();

    void AttemptExit(bool force = false);

    bool isRunning() const { return this->running; }
    std::thread::id getMainThreadId() const { return this->mainThreadId; }

    GLFWwindow* getGLFWWindowHandle() {
        if (UNLIKELY(!this->glfwWindowHndl)) {
            Logging::err << "[App::getGLFWWindowHandle] GLFW window handle does not exist (anymore)!" << std::endl;
            TRAP();
        }

        return this->glfwWindowHndl;
    }

private:
    void Menubar();

private:
    bool running { true };

    GLFWwindow* glfwWindowHndl { nullptr };

    bool isWindowMaximized { false };

    template <typename T>
    struct AppWindow {
    public:
        AppWindow() {
            static_assert(std::is_base_of<BaseWindow, T>::value, "T must be derived from BaseWindow");
            this->windowInstance = std::make_unique<T>();
        }

    public:
        bool shy { false };
        std::unique_ptr<T> windowInstance;

    public:
        void Update() {
            if (this->shy)
                return;

            if (UNLIKELY(!this->windowInstance)) {
                Logging::err << "[AppWindow<" << typeid(T).name() << ">::Update] Window instance does not exist (anymore)!" << std::endl;
                TRAP();
            }

            this->windowInstance->Update();
        }

        void Destroy() {
            this->windowInstance.reset();
        }
    };

    AppWindow<WindowCanvas> windowCanvas;
    AppWindow<WindowHybridList> windowHybridList;
    AppWindow<WindowInspector> windowInspector;
    AppWindow<WindowTimeline> windowTimeline;
    AppWindow<WindowSpritesheet> windowSpritesheet;

    AppWindow<WindowConfig> windowConfig;
    AppWindow<WindowAbout> windowAbout;

    AppWindow<WindowImguiDemo> windowDemo;

    std::thread::id mainThreadId;
};

#endif // TOAST_HPP
