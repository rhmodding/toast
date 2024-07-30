#ifndef APP_HPP
#define APP_HPP

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include "common.hpp"

#include "window/BaseWindow.hpp"

#include "window/WindowCanvas.hpp"
#include "window/WindowHybridList.hpp"
#include "window/WindowInspector.hpp"
#include "window/WindowTimeline.hpp"
#include "window/WindowSpritesheet.hpp"

#include "window/WindowConfig.hpp"
#include "window/WindowAbout.hpp"

#include <thread>

#define WINDOW_TITLE "Toast Beta"

class App {
public:
    App();
    ~App();

public:
    void Update();

    void AttemptExit(bool force = false);

    bool isRunning() const { return this->running; }

    GLFWwindow* getWindow() {
        if (UNLIKELY(!this->window)) {
            std::cerr << "[App::getWindow] Window ptr is nullptr !\n";
            __builtin_trap();
        }

        return this->window;
    }

private: // Methods
    void SetupFonts();

    void BeginMainWindow() {
        ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiWindowFlags windowFlags =
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));

        if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
            windowFlags |= ImGuiWindowFlags_NoBackground;

        ImGui::Begin(WINDOW_TITLE "###AppWindow", nullptr, windowFlags);

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();

        // Submit the Dockspace
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
            ImGui::DockSpace(ImGui::GetID("mainDockspace"), ImVec2(0.f, 0.f), dockspaceFlags);
    }

    void Menubar();

    void UpdatePopups();

    void UpdateFileDialogs();

    void HandleShortcuts();

private: // Windows
    template <typename T>
    struct AppWindow {
        bool shy{ false };
        std::unique_ptr<BaseWindow> window;

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

    GLFWwindow* window{ nullptr };

private: // Flags
    bool running{ true };

    // Launch dialog ###AttemptExitWhileUnsavedChanges
    bool dialog_warnExitWithUnsavedChanges{ false };
    // Exit even if there are unsaved changes.
    bool exitWithUnsavedChanges{ false };

private:
    std::thread::id windowThreadID;
public:
    std::thread::id getWindowThreadID() {
        return this->windowThreadID;
    }

}; // class App

extern App* gAppPtr;

#endif // APP_HPP
