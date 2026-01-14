#ifndef TOAST_HPP
#define TOAST_HPP

#include <imgui.h>

#include "glInclude.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <thread>

#include <stdexcept>

#include "window/WindowRoot.hpp"

#include "util/CxxDemangleUtil.hpp"

#include "Logging.hpp"

#include "Macro.hpp"

class Toast {
private:
    static Toast *gInstance;
public:
    static Toast *getInstance();

public:
    Toast(int argc, const char **argv);
    ~Toast();

    Toast(const Toast &) = delete;
    Toast &operator=(const Toast &) = delete;

public:
    void update();
    void draw();

    void requestExit(bool force = false);

    bool getRunning() const { return mRunning; }
    std::thread::id getMainThreadId() const { return mMainThreadId; }

    GLFWwindow *getGLFWWindowHandle() {
        if (UNLIKELY(mGlfwWindowHndl == nullptr)) {
            throw std::runtime_error("App::getGLFWWindowHandle: GLFW window handle does not exist (anymore)!");
        }

        return mGlfwWindowHndl;
    }

private:
    bool mRunning { true };

    GLFWwindow *mGlfwWindowHndl { nullptr };
    ImGuiContext *mGuiContext { nullptr };

    bool mIsWindowMaximized { false };

    bool mDrawnThisFrame { false };

    WindowRoot *mRootWindow { nullptr };

    std::thread::id mMainThreadId;
};

#endif // TOAST_HPP
