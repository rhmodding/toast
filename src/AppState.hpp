#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "anim/Animatable.hpp"

#include <cstdint>

#include "imgui.h"
#include "imgui_internal.h"

#include <chrono>

// Stores instance of AppState in local appState.
#define GET_APP_STATE AppState& appState = AppState::getInstance()

// Stores globalAnimatable refpointer in local globalAnimatable.
#define GET_ANIMATABLE Animatable*& globalAnimatable = AppState::getInstance().globalAnimatable

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

public:
    bool enableDemoWindow{ false };

    bool darkTheme{ true };
    ImVec4 windowClearColor{ Common::RGBAtoImVec4(24, 24, 24, 255) };
    void UpdateTheme();

    double updateRate{ 1000 / 60.0 };
    void UpdateUpdateRate(); // huh

    struct PlayerState {
        bool loopEnabled{ false };
        bool playing{ false };

        // Current frame
        uint16_t currentFrame{ 0 };

        uint16_t holdFramesLeft{ 0 };

        // Total frame count
        uint16_t frameCount{ 0 };

        uint16_t frameRate{ 60 };

        void updateSetFrameCount();
        void updateCurrentFrame();

        void ResetTimer();

        void ToggleAnimating(bool animating);

        uint16_t getTotalPseudoFrames();
        uint16_t getCurrentPseudoFrames();

        float getAnimationProgression();

        void Update();
    private:
        std::chrono::steady_clock::time_point previous;
        float timeLeft{ 0.f };
    } playerState;

    ImGuiID globalPopupID{ ImHashStr("GlobalPopupID") };

    ImFont* fontNormal;
    ImFont* fontLarge;
    ImFont* fontIcon;

    bool arrangementMode{ false };
    // This key object is meant for the arrangement mode to control the state of the key.
    RvlCellAnim::AnimationKey controlKey{ 0, 1, 1.f, 1.f, 0.f, 255 };

    uint16_t selectedAnimation{ 0 };
    // Apparently there can be 0 parts in an arrangement. In that case we set selectedPart to -1
    int32_t selectedPart{ -1 };

    bool focusOnSelectedPart{ false };
    
    Animatable* globalAnimatable{ nullptr };

private:
    AppState() {} // Private constructor to prevent instantiation
};

#endif // APPSTATE_HPP