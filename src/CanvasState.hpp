#ifndef CANVASSTATE_HPP
#define CANVASSTATE_HPP

#include <imgui.h>

#include <algorithm>

#include "ThemeManager.hpp"

enum CanvasGridType {
    CANVAS_GRID_TYPE_NONE,
    CANVAS_GRID_TYPE_DARK,
    CANVAS_GRID_TYPE_LIGHT,

    CANVAS_GRID_TYPE_USER
};

struct CanvasState {
public:
    static constexpr float MIN_ZOOM_FACTOR = .1f;
    static constexpr float MAX_ZOOM_FACTOR = 10.f;

    ImVec2 offset { 0.f, 0.f };
    float zoomFactor { 1.f };

    CanvasGridType gridType;
    ImVec4 userGridColor { 1.f, 1.f, 1.f, 1.f };
    bool gridLinesEnable { true };

    bool safeAreaEnable { false };
    bool safeAreaStereoscopic { false };
    uint8_t safeAreaAlpha { 255 };

    bool allowTransparentDraw { true };

    bool drawPartBounding { false };
    ImVec4 partBoundingDrawColor { 1.f, 1.f, 1.f, 1.f };

public:
    void resetView() {
        this->offset.x = 0.f;
        this->offset.y = 0.f;
        this->zoomFactor = 1.f;
    }

    void setDefaultGridType() {
        this->gridType = ThemeManager::getInstance().getThemeIsLight() ?
            CANVAS_GRID_TYPE_LIGHT : CANVAS_GRID_TYPE_DARK;
    }

    float getUserGridColorLumi() const {
        return
            .2126f * this->userGridColor.x +
            .7152f * this->userGridColor.y +
            .0722f * this->userGridColor.z;
    }

    void clampZoomFactor() {
        this->zoomFactor = std::clamp<float>(this->zoomFactor, MIN_ZOOM_FACTOR, MAX_ZOOM_FACTOR);
    }
};

#endif // CANVASSTATE_HPP
