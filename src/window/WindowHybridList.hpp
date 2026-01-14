#ifndef WINDOW_HYBRID_LIST_HPP
#define WINDOW_HYBRID_LIST_HPP

#include "BaseWindow.hpp"

class WindowHybridList : public BaseWindow {
public:
    void update() override;

private:
    void flashWindow();
    void resetFlash();

private:
    bool  mFlashWindow  { false };
    bool  mFlashTrigger { false };
    float mFlashTimer   { 0.f };
};

#endif // WINDOW_HYBRID_LIST_HPP
