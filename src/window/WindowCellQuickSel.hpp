#ifndef WINDOW_CELL_QUICK_SEL_HPP
#define WINDOW_CELL_QUICK_SEL_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include "Session.hpp"

class WindowCellQuickSel : public BaseWindow {
public:
    void update() override;

    void setOpen(bool open) override {
        mOpen = open;
    }

public:
    bool mOpen { false };
    int mCellSize { 94 };
    bool mCenterOnApply { true };
    Session::SavedCell mNewCell;
};

#endif // WINDOW_CELL_QUICK_SEL_HPP
