#ifndef THEME_MANAGER_HPP
#define THEME_MANAGER_HPP

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
        ImFont* normalJP { nullptr };
        ImFont* icon { nullptr };
    };

    void Initialize();

    void applyTheming();

    const std::array<float, 4>& getWindowClearColor() const {
        return mWindowClearColor;
    }

    bool getThemeIsLight() const;

private:
    FontCollection mFonts;

    std::array<float, 4> mWindowClearColor;
};

#endif // THEME_MANAGER_HPP
