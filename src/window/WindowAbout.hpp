#ifndef WINDOWABOUT_HPP
#define WINDOWABOUT_HPP

#include "BaseWindow.hpp"

class WindowAbout : public BaseWindow {
public:
    void Update() override;

    bool open{ false };
};

#endif // WINDOWABOUT_HPP