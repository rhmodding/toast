#include "ThemeManager.hpp"

#include "BIN/fontdata/FontAwesome.h"
#include "BIN/fontdata/NotoSans.h"
#include "BIN/fontdata/NotoSansJP.h"

#include "font/FontAwesome.h"

#include "ConfigManager.hpp"

#include "MainThreadTaskManager.hpp"

#include "Logging.hpp"

void ThemeManager::Initialize() {
    ImGuiIO& io = ImGui::GetIO();

    { // normal: Noto Sans (18px)
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        //fontConfig.OversampleH = 1;

        mFonts.normal = io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(NotoSans_data), NotoSans_length, 18.f, &fontConfig
        );
    }

    { // normal: Noto Sans JP (18px)
        ImFontConfig fontConfig;
        fontConfig.MergeMode = true;
        fontConfig.FontDataOwnedByAtlas = false;
        //fontConfig.OversampleH = 1;

        mFonts.normalJP = io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(NotoSansJP_data), NotoSansJP_length, 18.f, &fontConfig,
            io.Fonts->GetGlyphRangesJapanese()
        );
    }

    { // icon: Font Awesome
        static const ImWchar range[] { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig fontConfig;
        fontConfig.MergeMode = true;
        fontConfig.PixelSnapH = true;

        mFonts.icon = io.Fonts->AddFontFromMemoryCompressedBase85TTF(
            FONT_ICON_BUFFER_NAME_FA, 15.f, &fontConfig, range
        );
    }

    { // large: Noto Sans (24px)
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        //fontConfig.OversampleH = 1;

        mFonts.large = io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(NotoSans_data), NotoSans_length, 24.f, &fontConfig
        );
    }

    { // giant: Noto Sans (52px)
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        //fontConfig.OversampleH = 1;

        mFonts.giant = io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(NotoSans_data), NotoSans_length, 52.f, &fontConfig
        );
    }
}

void ThemeManager::applyTheming() {
    const auto& config = ConfigManager::getInstance().getConfig();

    switch (config.theme) {
    case ThemeChoice::Dark:
        mWindowClearColor = std::array<float, 4>({ 24 / 255.f, 24 / 255.f, 24 / 255.f, 1.f });
        break;
    case ThemeChoice::Light:
        mWindowClearColor = std::array<float, 4>({ 248 / 255.f, 248 / 255.f, 248 / 255.f, 1.f });
        break;
    default:
        Logging::warn << "[ThemeManager::applyTheming] Invalid theme: " << static_cast<int>(config.theme) << std::endl;
        break;
    }

    MainThreadTaskManager::getInstance().QueueTask([theme = config.theme]() {
        switch (theme) {
        case ThemeChoice::Dark:
            ImGui::StyleColorsDark();
            break;
        case ThemeChoice::Light:
            ImGui::StyleColorsLight();
            break;
        default:
            break;
        }

        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_WindowBg].w = 1.f;

        style.Colors[ImGuiCol_TabSelectedOverline].w = 0.f;
        style.Colors[ImGuiCol_TabDimmedSelectedOverline].w = 0.f;

        style.WindowPadding = { 10.f, 10.f };
        style.FramePadding  = { 5.f, 3.f };

        style.ItemSpacing = { 9.f, 4.f };

        // Center.
        style.WindowTitleAlign = { .5f, .5f };

        style.WindowRounding = 5.f;
        style.ChildRounding  = 5.f;
        style.PopupRounding  = 5.f;
        style.FrameRounding  = 5.f;
        style.GrabRounding   = 5.f;
        style.TabRounding    = 5.f;
    });
}

bool ThemeManager::getThemeIsLight() const {
    return ConfigManager::getInstance().getConfig().theme == ThemeChoice::Light;
}
