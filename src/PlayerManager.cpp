#include "anim/RvlCellAnim.hpp"

#include <numeric>

#include "PlayerManager.hpp"

#include "AppState.hpp"

void PlayerManager::setCurrentKeyIndex(unsigned index) {
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
        int p = appState.getMatchingNamePartIndex(
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

unsigned PlayerManager::getTotalPseudoFrames() {
    const auto& keys =
        AppState::getInstance().globalAnimatable->getCurrentAnimation()->keys;

    return std::accumulate(
        keys.begin(), keys.end(), 0u,
        [](unsigned sum, const RvlCellAnim::AnimationKey& key) {
            return sum + key.holdFrames;
        }
    );
}

unsigned PlayerManager::getElapsedPseudoFrames() {
    GET_ANIMATABLE;

    if (!this->playing && this->currentFrame == 0 && globalAnimatable->getHoldFramesLeft() < 1)
        return 0;

    unsigned result = std::accumulate(
        globalAnimatable->getCurrentAnimation()->keys.begin(),
        globalAnimatable->getCurrentAnimation()->keys.begin() + this->currentFrame,
        0u,
        [](unsigned sum, const RvlCellAnim::AnimationKey& key) {
            return sum + key.holdFrames;
        }
    );

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
            int p = appState.getMatchingNamePartIndex(
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
