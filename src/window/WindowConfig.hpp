#ifndef WINDOWCONFIG_HPP
#define WINDOWCONFIG_HPP

#include "BaseWindow.hpp"

#include "../ConfigManager.hpp"

class WindowConfig : public BaseWindow {
public:
    void Update() override;

    bool open { false };

    Config selfConfig;

private:
    bool firstOpen { true };
};

#endif // WINDOWCONFIG_HPP
