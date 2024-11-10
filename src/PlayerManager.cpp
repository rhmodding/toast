#include "anim/RvlCellAnim.hpp"

#include <numeric>

#include "PlayerManager.hpp"

#include "AppState.hpp"

void PlayerManager::setCurrentKeyIndex(unsigned index) {
    GET_APP_STATE;

    const auto* arrangeA = appState.globalAnimatable->getCurrentArrangement();

    appState.globalAnimatable->setAnimationKeyFromIndex(index);

    const auto* arrangeB = appState.globalAnimatable->getCurrentArrangement();

    if (arrangeA != arrangeB) {
        for (auto& part : appState.selectedParts) {
            int p = appState.getMatchingNamePartIndex(
                arrangeA->parts.at(part.index), *arrangeB
            );

            if (p >= 0)
                part.index = p;
        }

        appState.correctSelectedParts();
    }
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
        const auto* arrangeA = appState.globalAnimatable->getCurrentArrangement();

        globalAnimatable->Update();

        const auto* arrangeB = appState.globalAnimatable->getCurrentArrangement();

        delta -= this->timeLeft;
        this->timeLeft = 1.f / frameRate;

        if (!globalAnimatable->getAnimating() && this->looping) {
            globalAnimatable->setAnimationFromIndex(globalAnimatable->getCurrentAnimationIndex());
            globalAnimatable->setAnimating(true);
        }

        this->playing = globalAnimatable->getAnimating();
        this->currentFrame = globalAnimatable->getCurrentKeyIndex();

        if (arrangeA != arrangeB) {
            for (auto& part : appState.selectedParts) {
                int p = appState.getMatchingNamePartIndex(
                    arrangeA->parts.at(part.index), *arrangeB
                );

                if (p >= 0)
                    part.index = p;
            }

            appState.correctSelectedParts();
        }
    }

    this->timeLeft -= delta;
}
