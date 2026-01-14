#ifndef WINDOW_ABOUT_HPP
#define WINDOW_ABOUT_HPP

#include "BaseWindow.hpp"

#include "texture/Texture.hpp"

class WindowAbout : public BaseWindow {
public:
    WindowAbout();

    void update() override;
    void setOpen(bool open) override {
        mOpen = open;
    }

public:
    bool mOpen { false };

private:
    Texture mImage;
};

#endif // WINDOW_ABOUT_HPP
