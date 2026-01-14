#ifndef WINDOW_CONFIG_HPP
#define WINDOW_CONFIG_HPP

#include "BaseWindow.hpp"

#include "manager/ConfigManager.hpp"

class WindowConfig : public BaseWindow {
public:
    void update() override;

    void setOpen(bool open) override {
        mOpen = open;
    }

public:
    bool mOpen { false };

    Config mMyConfig;

private:
    bool mFirstOpen { true };
};

#endif // WINDOW_CONFIG_HPP
