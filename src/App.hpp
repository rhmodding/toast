#ifndef APP_HPP
#define APP_HPP

#include <imgui.h>
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

#include "common.hpp"

#define WINDOW_TITLE "Toast Beta"

class App {
public:
    App();
    ~App();

public:
    void Update();

    void AttemptExit(bool force = false);

    inline bool isRunning() const {
        return this->running;
    }

    GLFWwindow* getWindow() {
        if (UNLIKELY(!this->window)) {
            std::cerr << "[App::getWindow] Window ptr is nullptr !\n";
            __builtin_trap();
        }

        return this->window;
    }

private: // Methods
    void SetupFonts();

    void Menubar();

private: // Windows
    template <typename T>
    struct AppWindow {
        bool shy{ false };
        std::unique_ptr<T> window;

        void Update() {
            if (shy || UNLIKELY(!this->window))
                return;

            window->Update();
        }

        AppWindow() {
            static_assert(std::is_base_of<BaseWindow, T>::value, "T must be derived from BaseWindow");
            this->window = std::make_unique<T>();
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

    GLFWwindow* window{ nullptr };

private: // Flags
    bool running{ true };

private:
    std::thread::id mainThreadId;
public:
    std::thread::id getMainThreadId() {
        return this->mainThreadId;
    }

}; // class App

extern App* gAppPtr;

#endif // APP_HPP
