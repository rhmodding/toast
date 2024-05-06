#ifndef WINDOWCONFIG_HPP
#define WINDOWCONFIG_HPP

#include "BaseWindow.hpp"

#include "../ConfigManager.hpp"

class WindowConfig : public BaseWindow {
public:
    void Update() override;

    bool open{ false };
    
    ConfigManager::Config selfConfig;

    WindowConfig() { selfConfig = ConfigManager::getInstance().config; }
};

#endif // WINDOWCONFIG_HPP