#ifndef WINDOWABOUT_HPP
#define WINDOWABOUT_HPP

#include "BaseWindow.hpp"

#include "../texture/Texture.hpp"

class WindowAbout : public BaseWindow {
public:
    void Update() override;

public:
    bool mOpen { false };

private:
    Texture mImage;
};

#endif // WINDOWABOUT_HPP
