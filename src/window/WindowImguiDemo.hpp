#ifndef WINDOWIMGUIDEMO_HPP
#define WINDOWIMGUIDEMO_HPP

#include "BaseWindow.hpp"

class WindowImguiDemo : public BaseWindow {
public:
    void Update() override;

public:
    bool mOpen { false };
};

#endif // WINDOWIMGUIDEMO_HPP
