#include "WindowImGuiDemo.hpp"

#include <imgui.h>

void WindowImGuiDemo::update() {
    if (mOpen) {
        ImGui::ShowDemoWindow(&mOpen);
    }
}
