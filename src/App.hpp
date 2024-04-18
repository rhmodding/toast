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

#define WINDOW_TITLE "Sliced v1"

#define CANVAS_ZOOM_SPEED 0.04f
#define CANVAS_ZOOM_SPEED_FAST 0.08f

class App {
public:
    void Update();

    void Stop();

    // Setup
    App();

    GLFWwindow* getWindow() {
        assert(this->window);
        return this->window;
    }

    uint8_t quit{ 0 };

private:
    void UpdateTheme() {
        GET_APP_STATE;

        if (appState.darkTheme) {
            ImGui::StyleColorsDark();
            appState.windowClearColor = ImVec4((24 / 255.f), (24 / 255.f), (24 / 255.f), 1.f);
        }
        else {
            ImGui::StyleColorsLight();
            appState.windowClearColor = ImVec4((248 / 255.f), (248 / 255.f), (248 / 255.f), 1.f);
        }  
    }

    void SetupFonts();

    void BeginMainWindow(ImGuiIO& io) {
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

        ImGui::Begin(WINDOW_TITLE, nullptr, windowFlags);

        ImGui::PopStyleVar(3);

        // Submit the Dockspace
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspaceId = ImGui::GetID("mainDockspace");
            ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

            ImGui::PopStyleColor();
        }
    }

    void Animations();
    void Canvas();
    void Inspector();
    void Menubar();

    GLFWwindow* window{ nullptr };

    struct FileTab {
        const char* name;
        bool open{ false };
    } fileTabs[32];

    Common::Image cellanimSheet;
    RevCellAnim::RevCellAnimObject* cellanim{ nullptr };
    Animatable* animatable{ nullptr };
    std::unordered_map<uint16_t, std::string> animationNames;
}; // class App

#endif // APP_HPP