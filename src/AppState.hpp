#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "anim/Animatable.hpp"

#include <cstdint>

#include "imgui.h"

// Stores instance of AppState in local appState.
#define GET_APP_STATE AppState& appState = AppState::getInstance()

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

        void setAnimatable(Animatable* ptr) {
            this->animatable = ptr;
        }

        void updateSetFrameCount() {
            this->frameCount = static_cast<uint16_t>(
                this->animatable->cellanim->animations.at(
                    this->animatable->getCurrentAnimationIndex()
                ).keys.size()
            );
        }

        void updateCurrentFrame() {
            if (this->currentFrame >= this->frameCount)
                this->currentFrame = this->frameCount - 1;

            this->animatable->setAnimationKey(this->currentFrame);

            this->holdFramesLeft = 0;
        }

        void ResetTimer() {
            this->timeLeft = 1 / (float)frameRate;
            this->previous = glfwGetTime();
        }

        void ToggleAnimating(bool animating) {
            this->playing = animating;
            this->animatable->setAnimating(animating);
        }

        uint16_t getTotalPseudoFrames() {
            uint16_t result{ 0 };

            for (auto key : this->animatable->getCurrentAnimation()->keys)
                result += key.holdFrames;

            return result;
        }

        uint16_t getCurrentPseudoFrames() {
            if (!this->playing && this->currentFrame == 0 && this->holdFramesLeft == 0)
                return 0;

            uint16_t result{ 0 };

            for (uint16_t keyIndex = 0; keyIndex < currentFrame; keyIndex++)
                result += this->animatable->getCurrentAnimation()->keys.at(keyIndex).holdFrames;

            result += this->animatable->getCurrentKey()->holdFrames - this->holdFramesLeft;

            return result;
        }

        float getAnimationProgression() {
            return this->getCurrentPseudoFrames() / static_cast<float>(this->getTotalPseudoFrames());
        }

        void Update() {
            if (playing && animatable) {
                double now = glfwGetTime();
                float delta = static_cast<float>(now - this->previous);
                this->previous = now;

                this->timeLeft -= delta;
                if (timeLeft <= 0.f) {
                    this->timeLeft = 1 / (float)frameRate;

                    this->animatable->Update();

                    if (!this->animatable->isAnimating() && this->loopEnabled) {
                        this->animatable->setAnimation(this->animatable->getCurrentAnimationIndex());
                        this->animatable->setAnimating(true);
                    }

                    this->playing = this->animatable->isAnimating();

                    this->currentFrame = this->animatable->getCurrentKeyIndex();
                    this->holdFramesLeft = this->animatable->getHoldFramesLeft();
                }
            }
        }
    private:
        Animatable* animatable;

        double previous{ 0.f };
        float timeLeft{ 0.f };
    } playerState;

    ImFont* fontNormal;
    ImFont* fontLarge;
    ImFont* fontIcon;

private:
    AppState() {} // Private constructor to prevent instantiation
};

#endif // APPSTATE_HPP