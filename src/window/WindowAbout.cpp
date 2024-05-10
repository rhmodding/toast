#include "WindowAbout.hpp"

#include <imgui.h>

void WindowAbout::Update() {
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::SetNextWindowSize({ 820, 420 });

    ImGui::Begin("About", &this->open, ImGuiWindowFlags_NoResize);

    ImGui::End();
}