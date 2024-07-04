#ifndef APP_HPP
#define APP_HPP

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <cassert>

#include "window/WindowCanvas.hpp"
#include "window/WindowHybridList.hpp"
#include "window/WindowInspector.hpp"
#include "window/WindowTimeline.hpp"
#include "window/WindowSpritesheet.hpp"

#include "window/WindowConfig.hpp"
#include "window/WindowAbout.hpp"

#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>

#define WINDOW_TITLE "Toast Beta"

class App {
public:
    void Update();

    void AttemptExit();

    bool shouldStopRunning() const { return this->stopMainLoop; }

    // Constructor
    App();
    // Destructor
    ~App();

    GLFWwindow* getWindow() {
        assert(this->window);
        return this->window;
    }

private: // Methods
    void SetupFonts();

    void BeginMainWindow() {
        ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));

        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
            windowFlags |= ImGuiWindowFlags_NoBackground;

        ImGui::Begin(WINDOW_TITLE "###MainAppWindow", nullptr, windowFlags);

        ImGui::PopStyleVar(3);

        // Submit the Dockspace
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspaceId = ImGui::GetID("mainDockspace");
            ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

            ImGui::PopStyleColor();
        }
    }

    void Menubar();

    void UpdatePopups();

    void UpdateFileDialogs();

    void HandleShortcuts();

private: // Windows
    WindowCanvas* windowCanvas;
    WindowHybridList* windowHybridList;
    WindowInspector* windowInspector;
    WindowTimeline* windowTimeline;
    WindowSpritesheet* windowSpritesheet;

    WindowConfig* windowConfig;
    WindowAbout* windowAbout;

    GLFWwindow* window{ nullptr };

private: // Flags
    bool stopMainLoop{ false };

    // Launch dialog ###AttemptExitWhileUnsavedChanges
    bool dialog_warnExitWithUnsavedChanges{ false };
    // Exit even if there are unsaved changes.
    bool exitWithUnsavedChanges{ false };
    
private:
    std::mutex mtcQueueMutex;
    std::condition_variable mtcCondition;
public:
    struct MtCommand {
        std::function<void()> func;
        std::promise<void> promise;
    };
    std::queue<MtCommand> mtCommandQueue;

    std::future<void> enqueueCommand(std::function<void()> func) {
        if (std::this_thread::get_id() == this->getWindowThreadID()) {
            func();

            std::promise<void> promise;
            promise.set_value();

            return promise.get_future();
        }

        std::lock_guard<std::mutex> lock(this->mtcQueueMutex);

        MtCommand command{ func, std::promise<void>() };
        auto future = command.promise.get_future();

        this->mtCommandQueue.push(std::move(command));

        this->mtcCondition.notify_one();

        return future;
    }

    // TODO: move MtCommand stuff into another class

private:
    std::thread::id windowThreadID;
public:
    std::thread::id getWindowThreadID() {
        return this->windowThreadID;
    }

}; // class App

extern App* gAppPtr;

#endif // APP_HPP