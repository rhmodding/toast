#ifndef WINDOW_SPRITESHEET_HPP
#define WINDOW_SPRITESHEET_HPP

#include "BaseWindow.hpp"

#include <memory>

#include <imgui.h>

#include "texture/TextureEx.hpp"

class WindowSpritesheet : public BaseWindow {
public:
    void update() override;

private:
    void RunEditor();

    void FormatPopup();

private:
    enum GridType {
        GridType_None,
        GridType_Dark,
        GridType_Light,

        GridType_Custom
    };
    GridType mGridType;

    ImVec4 mCustomGridColor { 1.f, 1.f, 1.f, 1.f };

    bool mDrawBounding { true };

    bool mSheetZoomEnabled { false };
    bool mSheetZoomTriggered { false };
    float mSheetZoomTimer { 0.f };

    float mUserSheetZoom { 1.f };
    ImVec2 mSheetZoomedOffset { 0.f, 0.f };

    bool mShowPaletteWindow { false };

    std::shared_ptr<TextureEx> mFormattingNewTex;
};

#endif // WINDOW_SPRITESHEET_HPP
