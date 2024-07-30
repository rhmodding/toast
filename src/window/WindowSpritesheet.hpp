#ifndef WINDOWSPRITESHEET_HPP
#define WINDOWSPRITESHEET_HPP

#include "BaseWindow.hpp"

#include "../AppState.hpp"

class WindowSpritesheet : public BaseWindow {
public:
    void Update() override;

    enum GridType : uint8_t {
        GridType_None,
        GridType_Dark,
        GridType_Light,

        GridType_Custom
    } gridType{ AppState::getInstance().getDarkThemeEnabled() ? GridType_Dark : GridType_Light };
    ImVec4 customGridColor{ 1.f, 1.f, 1.f, 1.f };

    bool drawBounding{ true };

private:
    bool sheetZoomEnabled{ false };
    bool sheetZoomTriggered{ false };
    float sheetZoomTimer{ 0.f };

    float userSheetZoom{ 1.f };

    ImVec2 sheetZoomedOffset{ 0.f, 0.f };

    bool showPaletteWindow{ false };

private:
    void RunEditor();

    void PaletteWindow();

    void FormatPopup();
};

#endif // WINDOWSPRITESHEET_HPP
