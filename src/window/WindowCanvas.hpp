#ifndef WINDOW_CANVAS_HPP
#define WINDOW_CANVAS_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include <cstdint>

#include "CanvasState.hpp"

#include "cellanim/CellAnimRenderer.hpp"

class WindowCanvas : public BaseWindow {
public:
    void Update() override;

private:
    void Menubar();

    void DrawCanvasText();

public:
    CanvasState mState;

private:
    CellAnimRenderer mCellAnimRenderer;

    ImVec2 mCanvasTopLeft;
    ImVec2 mCanvasSize;

    enum PartHandle : int {
        PartHandle_None = -1,
        PartHandle_Top = 0,
        PartHandle_Right = 1,
        PartHandle_Bottom = 2,
        PartHandle_Left = 3,
        PartHandle_TopLeft = 4,
        PartHandle_TopRight = 5,
        PartHandle_BottomRight = 6,
        PartHandle_BottomLeft = 7,
        PartHandle_Whole = 8
    } mHoveredPartHandle { PartHandle_None };
};

#endif // WINDOW_CANVAS_HPP
