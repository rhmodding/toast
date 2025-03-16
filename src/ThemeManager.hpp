#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include "Singleton.hpp"

#include <array>

#include <imgui.h>

class ThemeManager : public Singleton<ThemeManager> {
    friend class Singleton<ThemeManager>; // Allow access to base class constructor

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
        return this->windowClearColor;
    }
    const FontCollection& getFonts() const {
        return this->fonts;
    }

    bool getThemeIsLight() const;

private:
    FontCollection fonts;

    std::array<float, 4> windowClearColor;
};

#endif // THEMEMANAGER_HPP
