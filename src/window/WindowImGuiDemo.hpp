#ifndef WINDOW_IMGUI_DEMO_HPP
#define WINDOW_IMGUI_DEMO_HPP

#include "BaseWindow.hpp"

class WindowImGuiDemo : public BaseWindow {
public:
    void update() override;
    void setOpen(bool open) override {
        mOpen = open;
    }

public:
    bool mOpen { false };
};

#endif // WINDOW_IMGUI_DEMO_HPP
