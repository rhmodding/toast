#ifndef WINDOWCANVAS_HPP
#define WINDOWCANVAS_HPP

#include "imgui.h"

#include <cstdint>

#include "../AppState.hpp"

#include "BaseWindow.hpp"

#include "../anim/Animatable.hpp"

#define CANVAS_ZOOM_SPEED 0.04f
#define CANVAS_ZOOM_SPEED_FAST 0.08f

class WindowCanvas : public BaseWindow {
public:
    void Update() override;

    ImVec2 canvasOffset{ 0.f, 0.f };

    bool   drawPartOrigin{ false };
    ImVec4 partOriginDrawColor{ 1.f, 1.f, 1.f, 1.f };
    float  partOriginDrawRadius{ 5.f };

    bool   allowOpacity{ true };

    float  canvasZoom{ 0.f };

    enum GridType : uint8_t {
        GridType_None,
        GridType_Dark,
        GridType_Light,
        
        GridType_Custom
    } gridType{ AppState::getInstance().darkTheme ? GridType_Dark : GridType_Light };
    ImVec4 customGridColor{ 1.f, 1.f, 1.f, 1.f };
    bool   enableGridLines{ true };


    bool    visualizeSafeArea{ false };
    uint8_t safeAreaAlpha{ 255 };

private:

    void Menubar();
};

#endif // WINDOWCANVAS_HPP