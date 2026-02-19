#include "WindowImGuiDemo.hpp"

#include <imgui.h>

void WindowImGuiDemo::update() {
    ImGui::ShowDemoWindow(&mOpen);
}
