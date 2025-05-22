#ifndef WINDOWCONFIG_HPP
#define WINDOWCONFIG_HPP

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

#endif // WINDOWCONFIG_HPP
