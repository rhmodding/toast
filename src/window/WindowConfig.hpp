#ifndef WINDOWCONFIG_HPP
#define WINDOWCONFIG_HPP

#include "BaseWindow.hpp"

#include "../ConfigManager.hpp"

class WindowConfig : public BaseWindow {
public:
    void Update() override;

    bool open{ false };

    ConfigManager::Config selfConfig;
};

#endif // WINDOWCONFIG_HPP
