#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "anim/Animatable.hpp"

#include <cstdint>

#include "imgui.h"

// Stores instance of AppState in local appState.
#define GET_APP_STATE AppState& appState = AppState::getInstance()

#define GET_ANIMATABLE Animatable*& globalAnimatable = AppState::getInstance().globalAnimatable

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

public:
    bool enableDemoWindow{ true };
    bool showAboutWindow{ false };

    uint8_t darkTheme{ 0xff };
    ImVec4 windowClearColor{ ImVec4((24 / 255.f), (24 / 255.f), (24 / 255.f), 1.f) };

    struct PlayerState {
        bool loopEnabled{ false };
        bool playing{ false };

        // Current frame
        uint16_t currentFrame{ 0 };

        uint16_t holdFramesLeft{ 0 };

        // Total frame count
        uint16_t frameCount{ 0 };

        uint16_t frameRate{ 60 };

        void updateSetFrameCount() {
            GET_ANIMATABLE;

            this->frameCount = static_cast<uint16_t>(
                globalAnimatable->cellanim->animations.at(
                    globalAnimatable->getCurrentAnimationIndex()
                ).keys.size()
            );
        }

        void updateCurrentFrame() {
            GET_ANIMATABLE;

            if (this->currentFrame >= this->frameCount)
                this->currentFrame = this->frameCount - 1;

            globalAnimatable->setAnimationKey(this->currentFrame);

            this->holdFramesLeft = 0;
        }

        void ResetTimer() {
            this->timeLeft = 1 / (float)frameRate;
            this->previous = glfwGetTime();
        }

        void ToggleAnimating(bool animating) {
            GET_ANIMATABLE;

            this->playing = animating;
            globalAnimatable->setAnimating(animating);
        }

        uint16_t getTotalPseudoFrames() {
            uint16_t result{ 0 };

            GET_ANIMATABLE;

            for (auto key : globalAnimatable->getCurrentAnimation()->keys)
                result += key.holdFrames;

            return result;
        }

        uint16_t getCurrentPseudoFrames() {
            if (!this->playing && this->currentFrame == 0 && this->holdFramesLeft == 0)
                return 0;

            uint16_t result{ 0 };

            GET_ANIMATABLE;

            for (uint16_t keyIndex = 0; keyIndex < currentFrame; keyIndex++)
                result += globalAnimatable->getCurrentAnimation()->keys.at(keyIndex).holdFrames;

            result += globalAnimatable->getCurrentKey()->holdFrames - this->holdFramesLeft;

            return result;
        }

        float getAnimationProgression() {
            return this->getCurrentPseudoFrames() / static_cast<float>(this->getTotalPseudoFrames());
        }

        void Update() {
            GET_ANIMATABLE;

            if (playing && globalAnimatable) {
                double now = glfwGetTime();
                float delta = static_cast<float>(now - this->previous);
                this->previous = now;

                this->timeLeft -= delta;
                if (timeLeft <= 0.f) {
                    this->timeLeft = 1 / (float)frameRate;

                    globalAnimatable->Update();

                    if (!globalAnimatable->isAnimating() && this->loopEnabled) {
                        globalAnimatable->setAnimation(globalAnimatable->getCurrentAnimationIndex());
                        globalAnimatable->setAnimating(true);
                    }

                    this->playing = globalAnimatable->isAnimating();

                    this->currentFrame = globalAnimatable->getCurrentKeyIndex();
                    this->holdFramesLeft = globalAnimatable->getHoldFramesLeft();
                }
            }
        }
    private:
        double previous{ 0.f };
        float timeLeft{ 0.f };
    } playerState;

    ImFont* fontNormal;
    ImFont* fontLarge;
    ImFont* fontIcon;

    bool arrangementMode{ false };
    RvlCellAnim::AnimationKey controlKey{ 0, 1, 1.f, 1.f, 0.f, 255 };

    uint16_t selectedAnimation{ 0 };
    uint16_t selectedPart{ 0 };
    bool drawSelectedPartBounding{ false };
    
    Animatable* globalAnimatable{ nullptr };

private:
    AppState() {} // Private constructor to prevent instantiation
};

#endif // APPSTATE_HPP