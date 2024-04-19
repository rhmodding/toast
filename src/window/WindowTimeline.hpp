#ifndef WINDOWTIMELINE_HPP
#define WINDOWTIMELINE_HPP

#include "BaseWindow.hpp"

#include "../anim/Animatable.hpp"

class WindowTimeline : public BaseWindow {
public:
    void Update() override;

    Animatable* animatable;
};

#endif // WINDOWTIMELINE_HPP