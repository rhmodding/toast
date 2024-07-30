#ifndef WINDOWABOUT_HPP
#define WINDOWABOUT_HPP

#include "BaseWindow.hpp"

#include "../common.hpp"

class WindowAbout : public BaseWindow {
public:
    void Update() override;

    bool open{ false };

private:
    Common::Image image;
};

#endif // WINDOWABOUT_HPP
