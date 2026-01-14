#include "WindowImGuiDemo.hpp"

#include <imgui.h>

void WindowImGuiDemo::update() {
    if (!mOpen) {
        return;
    }

    ImGui::ShowDemoWindow(&mOpen);
}
