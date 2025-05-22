#include "WindowImguiDemo.hpp"

#include <imgui.h>

void WindowImguiDemo::Update() {
    if (!mOpen) {
        return;
    }

    ImGui::ShowDemoWindow(&mOpen);
}
