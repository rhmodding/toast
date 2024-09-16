#ifndef WINDOWIMGUIDEMO_HPP
#define WINDOWIMGUIDEMO_HPP

#include "BaseWindow.hpp"

class WindowImguiDemo : public BaseWindow {
public:
    void Update() override;

    bool open{ false };
};

#endif // WINDOWIMGUIDEMO_HPP
