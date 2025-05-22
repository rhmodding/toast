#ifndef WINDOWHYBRIDLIST_HPP
#define WINDOWHYBRIDLIST_HPP

#include "BaseWindow.hpp"

class WindowHybridList : public BaseWindow {
public:
    void Update() override;

private:
    void flashWindow();
    void resetFlash();

private:
    bool  mFlashWindow  { false };
    bool  mFlashTrigger { false };
    float mFlashTimer   { 0.f };
};

#endif // WINDOWHYBRIDLIST_HPP
