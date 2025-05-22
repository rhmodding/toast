#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include "Singleton.hpp"

#include <array>

#include <imgui.h>

class ThemeManager : public Singleton<ThemeManager> {
    friend class Singleton<ThemeManager>;

private:
    ThemeManager() = default;
public:
    ~ThemeManager() = default;

    struct FontCollection {
        ImFont* normal { nullptr };
        ImFont* large { nullptr };
        ImFont* giant { nullptr };
        ImFont* icon { nullptr };
    };

    void Initialize();

    void applyTheming();

    const std::array<float, 4>& getWindowClearColor() const {
        return mWindowClearColor;
    }
    const FontCollection& getFonts() const {
        return mFonts;
    }

    bool getThemeIsLight() const;

private:
    FontCollection mFonts;

    std::array<float, 4> mWindowClearColor;
};

#endif // THEMEMANAGER_HPP
