#include "anim/RvlCellAnim.hpp"

#include "PlayerManager.hpp"

#include "AppState.hpp"

void PlayerManager::setCurrentKeyIndex(uint16_t index) {
    GET_APP_STATE;

    RvlCellAnim::ArrangementPart* part{ nullptr };
    if (
        appState.selectedPart >= 0 &&
        appState.selectedPart <
            appState.globalAnimatable->getCurrentArrangement()->parts.size()
    )
        part = &appState.globalAnimatable->getCurrentArrangement()->
            parts.at(appState.selectedPart);

    appState.globalAnimatable->setAnimationKeyFromIndex(index);

    if (part) {
        int32_t p = appState.getMatchingNamePartIndex(
            *part,
            *appState.globalAnimatable->getCurrentArrangement()
        );

        if (p >= 0)
            appState.selectedPart = p;
    }

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

uint32_t PlayerManager::getTotalPseudoFrames() {
    GET_ANIMATABLE;

    uint32_t pseudoFrames{ 0 };
    for (const auto& key : globalAnimatable->getCurrentAnimation()->keys)
        pseudoFrames += key.holdFrames;

    return pseudoFrames;
}

uint32_t PlayerManager::getElapsedPseudoFrames() {
    GET_ANIMATABLE;

    if (!this->playing && this->currentFrame == 0 && globalAnimatable->getHoldFramesLeft() < 1)
        return 0;

    uint32_t result{ 0 };
    for (uint16_t keyIndex = 0; keyIndex < this->currentFrame; keyIndex++)
        result += globalAnimatable->getCurrentAnimation()->keys.at(keyIndex).holdFrames;

    result += globalAnimatable->getCurrentKey()->holdFrames - globalAnimatable->getHoldFramesLeft();

    return result;
}

void PlayerManager::Update() {
    GET_APP_STATE;
    GET_ANIMATABLE;

    if (!this->playing || !globalAnimatable)
        return;

    auto now = std::chrono::steady_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - this->previous).count();

    this->previous = now;

    while (delta >= this->timeLeft) {
        RvlCellAnim::ArrangementPart* part{ nullptr };
        if (appState.selectedPart >= 0)
            part = &appState.globalAnimatable->getCurrentArrangement()->
                parts.at(appState.selectedPart);

        globalAnimatable->Update();

        delta -= this->timeLeft;
        this->timeLeft = 1.f / frameRate;

        if (!globalAnimatable->getAnimating() && this->looping) {
            globalAnimatable->setAnimationFromIndex(globalAnimatable->getCurrentAnimationIndex());
            globalAnimatable->setAnimating(true);
        }

        this->playing = globalAnimatable->getAnimating();
        this->currentFrame = globalAnimatable->getCurrentKeyIndex();

        if (part) {
            int32_t p = appState.getMatchingNamePartIndex(
                *part,
                *appState.globalAnimatable->getCurrentArrangement()
            );

            if (p >= 0)
                appState.selectedPart = p;
        }

        appState.correctSelectedPart();
    }

    this->timeLeft -= delta;
}
