#ifndef TOAST_HPP
#define TOAST_HPP

#include <imgui.h>

#include "glInclude.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <thread>

#include <stdexcept>

#include "window/BaseWindow.hpp"

#include "window/WindowCanvas.hpp"
#include "window/WindowHybridList.hpp"
#include "window/WindowInspector.hpp"
#include "window/WindowTimeline.hpp"
#include "window/WindowSpritesheet.hpp"
#include "window/WindowConfig.hpp"
#include "window/WindowAbout.hpp"
#include "window/WindowImguiDemo.hpp"

#include "CxxDemangle.hpp"

#include "Logging.hpp"

#include "common.hpp"

class Toast {
private:
    static Toast* gInstance;
public:
    static Toast* getInstance();

public:
    Toast(int argc, const char** argv);
    ~Toast();

    Toast(const Toast&) = delete;
    Toast& operator=(const Toast&) = delete;

public:
    void Update();
    void Draw();

    void AttemptExit(bool force = false);

    bool isRunning() const { return mRunning; }
    std::thread::id getMainThreadId() const { return mMainThreadId; }

    GLFWwindow* getGLFWWindowHandle() {
        if (UNLIKELY(mGlfwWindowHndl == nullptr))
            throw std::runtime_error("App::getGLFWWindowHandle: GLFW window handle does not exist (anymore)!");

        return mGlfwWindowHndl;
    }

private:
    void Menubar();

private:
    bool mRunning { true };

    GLFWwindow* mGlfwWindowHndl { nullptr };
    ImGuiContext* mGuiContext { nullptr };

    bool mIsWindowMaximized { false };

    template <typename T>
    struct ToastWindow {
    public:
        ToastWindow() {
            static_assert(std::is_base_of<BaseWindow, T>::value, "T must be derived from BaseWindow");
            this->windowInstance = std::make_unique<T>();
        }

    public:
        bool shy { false };
        std::unique_ptr<T> windowInstance;

    public:
        void Update() {
            if (this->shy) {
                return;
            }

            if (UNLIKELY(!this->windowInstance)) {
                throw std::runtime_error(
                    "ToastWindow<" + CxxDemangle::Demangle<T>() + ">::Update: Window instance does not exist (anymore)!"
                );
            }

            this->windowInstance->Update();
        }

        void Destroy() {
            this->windowInstance.reset();
        }

        void UIShyToggle(const char* label) {
            if (ImGui::MenuItem(label, "", !this->shy)) {
                this->shy ^= true;
            }
        }
    };

    ToastWindow<WindowCanvas> mWindowCanvas;
    ToastWindow<WindowHybridList> mWindowHybridList;
    ToastWindow<WindowInspector> mWindowInspector;
    ToastWindow<WindowTimeline> mWindowTimeline;
    ToastWindow<WindowSpritesheet> mWindowSpritesheet;

    ToastWindow<WindowConfig> mWindowConfig;
    ToastWindow<WindowAbout> mWindowAbout;

    ToastWindow<WindowImguiDemo> mWindowDemo;

    std::thread::id mMainThreadId;
};

#endif // TOAST_HPP
