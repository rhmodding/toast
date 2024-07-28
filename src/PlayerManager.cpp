#include "PlayerManager.hpp"

void PlayerManager::setCurrentKeyIndex(uint16_t index) {
    GET_APP_STATE;

    appState.globalAnimatable->setAnimationKeyFromIndex(index);

    appState.correctSelectedPart();
}

void PlayerManager::ResetTimer() {
    this->timeLeft = 1.f / this->frameRate;
    this->previous = std::chrono::steady_clock::now();
}

void PlayerManager::setAnimating(bool animating) {
    GET_ANIMATABLE;

    this->playing = animating;
    globalAnimatable->setAnimating(animating);
}

uint16_t PlayerManager::getTotalPseudoFrames() {
    GET_ANIMATABLE;

    uint16_t pseudoFrames{ 0 };
    for (const auto& key : globalAnimatable->getCurrentAnimation()->keys)
        pseudoFrames += key.holdFrames;

    return pseudoFrames;
}

uint16_t PlayerManager::getCurrentPseudoFrames() {
    GET_ANIMATABLE;

    if (!this->playing && this->currentFrame == 0 && globalAnimatable->getHoldFramesLeft() < 1)
        return 0;

    uint16_t result{ 0 };
    for (uint32_t keyIndex = 0; keyIndex < this->currentFrame; keyIndex++)
        result += globalAnimatable->getCurrentAnimation()->keys.at(keyIndex).holdFrames;

    result += globalAnimatable->getCurrentKey()->holdFrames - globalAnimatable->getHoldFramesLeft();

    return result;
}

void PlayerManager::Update() {
    GET_ANIMATABLE;

    if (!this->playing || !globalAnimatable)
        return;

    auto now = std::chrono::steady_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - this->previous).count();

    this->previous = now;

    while (delta >= this->timeLeft) {
        globalAnimatable->Update();

        delta -= this->timeLeft;
        this->timeLeft = 1.f / frameRate;

        if (!globalAnimatable->getAnimating() && this->looping) {
            globalAnimatable->setAnimationFromIndex(globalAnimatable->getCurrentAnimationIndex());
            globalAnimatable->setAnimating(true);
        }

        this->playing = globalAnimatable->getAnimating();
        this->currentFrame = globalAnimatable->getCurrentKeyIndex();

        AppState::getInstance().correctSelectedPart();
    }

    this->timeLeft -= delta;
}
