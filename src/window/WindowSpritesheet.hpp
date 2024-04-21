#ifndef WINDOWSPRITESHEET_HPP
#define WINDOWSPRITESHEET_HPP

#include "BaseWindow.hpp"

#include "../anim/Animatable.hpp"

#include "../common.hpp"

#include "../AppState.hpp"

class WindowSpritesheet : public BaseWindow {
public:
    void Update() override;

    enum GridType : uint8_t {
        GridType_None,
        GridType_Dark,
        GridType_Light,
        
        GridType_Custom
    } gridType{ AppState::getInstance().darkTheme ? GridType_Dark : GridType_Light };
    ImVec4 customGridColor{ 1.f, 1.f, 1.f, 1.f };

    bool drawBounding{ true };

private:
};

#endif // WINDOWSPRITESHEET_HPP