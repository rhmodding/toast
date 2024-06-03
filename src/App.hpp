#ifndef APP_HPP
#define APP_HPP

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "AppState.hpp"

#include <iostream>
#include <cassert>

#include <unordered_map>
#include <string>

#include "common.hpp"
#include "anim/Animatable.hpp"

#include "window/WindowCanvas.hpp"
#include "window/WindowHybridList.hpp"
#include "window/WindowInspector.hpp"
#include "window/WindowTimeline.hpp"
#include "window/WindowSpritesheet.hpp"

#include "window/WindowConfig.hpp"
#include "window/WindowAbout.hpp"

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

}; // class App

#endif // APP_HPP