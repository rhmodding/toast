#include "AppState.hpp"

void AppState::PlayerState::updateSetFrameCount() {
    GET_ANIMATABLE;

    this->frameCount = static_cast<uint16_t>(
        globalAnimatable->cellanim->animations.at(
            globalAnimatable->getCurrentAnimationIndex()
        ).keys.size()
    );
}

void AppState::PlayerState::updateCurrentFrame() {
    GET_ANIMATABLE;
    GET_APP_STATE;

    if (this->currentFrame >= this->frameCount)
        this->currentFrame = this->frameCount - 1;

    globalAnimatable->setAnimationKey(this->currentFrame);

    RvlCellAnim::Arrangement* arrangementPtr =
        &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

    if (appState.selectedPart >= arrangementPtr->parts.size())
        appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);

    this->holdFramesLeft = 0;
}

void AppState::PlayerState::ResetTimer() {
    this->timeLeft = 1 / (float)frameRate;
    this->previous = glfwGetTime();
}

void AppState::PlayerState::ToggleAnimating(bool animating) {
    GET_ANIMATABLE;

    this->playing = animating;
    globalAnimatable->setAnimating(animating);
}

uint16_t AppState::PlayerState::getTotalPseudoFrames() {
    uint16_t result{ 0 };

    GET_ANIMATABLE;

    for (auto key : globalAnimatable->getCurrentAnimation()->keys)
        result += key.holdFrames;

    return result;
}

uint16_t AppState::PlayerState::getCurrentPseudoFrames() {
    if (!this->playing && this->currentFrame == 0 && this->holdFramesLeft == 0)
        return 0;

    uint16_t result{ 0 };

    GET_ANIMATABLE;

    for (uint16_t keyIndex = 0; keyIndex < currentFrame; keyIndex++)
        result += globalAnimatable->getCurrentAnimation()->keys.at(keyIndex).holdFrames;

    result += globalAnimatable->getCurrentKey()->holdFrames - this->holdFramesLeft;

    return result;
}

float AppState::PlayerState::getAnimationProgression() {
    return this->getCurrentPseudoFrames() / static_cast<float>(this->getTotalPseudoFrames());
}

void AppState::PlayerState::Update() {
    GET_ANIMATABLE;

    if (this->playing && globalAnimatable) {
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

            GET_APP_STATE;

            RvlCellAnim::Arrangement* arrangementPtr =
                &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

            if (appState.selectedPart >= arrangementPtr->parts.size())
                appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);
        }
    }
}