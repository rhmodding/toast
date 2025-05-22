#ifndef WINDOW_CONFIG_HPP
#define WINDOW_CONFIG_HPP

#include "BaseWindow.hpp"

#include "../ConfigManager.hpp"

class WindowConfig : public BaseWindow {
public:
    void Update() override;

public:
    bool mOpen { false };

    Config mMyConfig;

private:
    bool mFirstOpen { true };
};

#endif // WINDOW_CONFIG_HPP
