#ifndef WINDOWABOUT_HPP
#define WINDOWABOUT_HPP

#include "BaseWindow.hpp"

#include "../texture/Texture.hpp"

class WindowAbout : public BaseWindow {
public:
    void Update() override;

    bool open { false };

private:
    Texture image;
};

#endif // WINDOWABOUT_HPP
