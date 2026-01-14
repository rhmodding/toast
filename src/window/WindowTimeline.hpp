#ifndef WINDOW_TIMELINE_HPP
#define WINDOW_TIMELINE_HPP

#include "BaseWindow.hpp"

class WindowTimeline : public BaseWindow {
public:
    void update() override;

private:
    void ChildToolbar();
    void ChildKeys();
};

#endif // WINDOW_TIMELINE_HPP
