#include "AppState.hpp"

#include "ConfigManager.hpp"

void AppState::UpdateTheme() {
    if (this->getDarkThemeEnabled()) {
        ImGui::StyleColorsDark();
        this->windowClearColor = Common::RGBAtoImVec4(24, 24, 24, 255);
    }
    else {
        ImGui::StyleColorsLight();
        this->windowClearColor = Common::RGBAtoImVec4(248, 248, 248, 255);
    }
}