#include "WindowImguiDemo.hpp"

#include <imgui.h>

void WindowImguiDemo::Update() {
    if (!this->open)
        return;

    ImGui::ShowDemoWindow(&this->open);
}
