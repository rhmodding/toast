#ifndef WINDOWCANVAS_HPP
#define WINDOWCANVAS_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include <cstdint>

#include "../CanvasState.hpp"

#include "../cellanim/CellAnimRenderer.hpp"

class WindowCanvas : public BaseWindow {
public:
    void Update() override;

    CanvasState state;
private:
    CellAnimRenderer cellanimRenderer;

    ImVec2 canvasTopLeft;
    ImVec2 canvasSize;

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
    } hoveredPartHandle { PartHandle_None };

    void Menubar();

    void DrawCanvasText();
};

#endif // WINDOWCANVAS_HPP
