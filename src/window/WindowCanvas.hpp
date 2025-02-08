#ifndef WINDOWCANVAS_HPP
#define WINDOWCANVAS_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include <cstdint>

class WindowCanvas : public BaseWindow {
public:
    void Update() override;

    ImVec2 canvasOffset { 0.f, 0.f };

    bool   drawAllBounding { false };
    ImVec4 partBoundingDrawColor { 1.f, 1.f, 1.f, 1.f };
    float  partOriginDrawRadius { 5.f };

    bool   allowOpacity { true };

    float  canvasZoom { 1.f };

    enum GridType {
        GridType_None,
        GridType_Dark,
        GridType_Light,

        GridType_Custom
    } gridType;
    ImVec4 customGridColor { 1.f, 1.f, 1.f, 1.f };
    bool   enableGridLines { true };

    bool    showSafeArea { false };
    uint8_t safeAreaAlpha { 255 };

private:
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
