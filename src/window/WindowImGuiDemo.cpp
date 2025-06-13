#include "WindowImGuiDemo.hpp"

#include <imgui.h>

void WindowImGuiDemo::Update() {
    if (!mOpen) {
        return;
    }

    ImGui::ShowDemoWindow(&mOpen);
}
