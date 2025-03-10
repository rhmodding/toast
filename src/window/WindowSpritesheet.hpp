#ifndef WINDOWSPRITESHEET_HPP
#define WINDOWSPRITESHEET_HPP

#include "BaseWindow.hpp"

#include <cstdint>

#include <memory>

#include <imgui.h>

#include "../texture/Texture.hpp"

class WindowSpritesheet : public BaseWindow {
public:
    void Update() override;

    enum GridType {
        GridType_None,
        GridType_Dark,
        GridType_Light,

        GridType_Custom
    } gridType;
    ImVec4 customGridColor { 1.f, 1.f, 1.f, 1.f };

    bool drawBounding { true };

private:
    bool sheetZoomEnabled { false };
    bool sheetZoomTriggered { false };
    float sheetZoomTimer { 0.f };

    float userSheetZoom { 1.f };

    ImVec2 sheetZoomedOffset { 0.f, 0.f };

    bool showPaletteWindow { false };

    std::shared_ptr<Texture> formattingNewTex;

private:
    void RunEditor();

    void FormatPopup();
};

#endif // WINDOWSPRITESHEET_HPP
