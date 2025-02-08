#include "anim/RvlCellAnim.hpp"

#include <numeric>

#include "PlayerManager.hpp"

#include "AppState.hpp"

void PlayerManager::setCurrentKeyIndex(unsigned index) {
    AppState& appState = AppState::getInstance();

    if (this->getCurrentKeyIndex() >= this->getKeyCount()) {
        appState.globalAnimatable.setAnimationKeyFromIndex(index);
        appState.correctSelectedParts();

        return;
    }

    const auto* arrangeBefore = appState.globalAnimatable.getCurrentArrangement();

    appState.globalAnimatable.setAnimationKeyFromIndex(index);

    const auto* arrangeAfter = appState.globalAnimatable.getCurrentArrangement();

    if (arrangeBefore != arrangeAfter) {
        for (auto& part : appState.selectedParts) {
            int p = appState.getMatchingNamePartIndex(
                arrangeBefore->parts.at(part.index), *arrangeAfter
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

void PlayerManager::setPlaying(bool animating) {
    this->playing = animating;
    AppState::getInstance().globalAnimatable.setAnimating(animating);
}

unsigned PlayerManager::getTotalPseudoFrames() const {
    const auto& keys =
        AppState::getInstance().globalAnimatable.getCurrentAnimation()->keys;

    return std::accumulate(
        keys.begin(), keys.end(), 0u,
        [](unsigned sum, const RvlCellAnim::AnimationKey& key) {
            return sum + key.holdFrames;
        }
    );
}

unsigned PlayerManager::getElapsedPseudoFrames() const {
    Animatable& globalAnimatable = AppState::getInstance().globalAnimatable;

    unsigned currentKeyIndex = this->getCurrentKeyIndex();

    if (!this->playing && currentKeyIndex == 0 && globalAnimatable.getHoldFramesLeft() < 1)
        return 0;

    unsigned result = std::accumulate(
        globalAnimatable.getCurrentAnimation()->keys.begin(),
        globalAnimatable.getCurrentAnimation()->keys.begin() + currentKeyIndex,
        0u,
        [](unsigned sum, const RvlCellAnim::AnimationKey& key) {
            return sum + key.holdFrames;
        }
    );

    result += globalAnimatable.getCurrentKey()->holdFrames - globalAnimatable.getHoldFramesLeft();

    return result;
}

void PlayerManager::Update() {
    AppState& appState = AppState::getInstance();
    Animatable& globalAnimatable = appState.globalAnimatable;

    if (!this->playing)
        return;

    auto now = std::chrono::steady_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - this->previous).count();

    this->previous = now;

    const auto* arrangeBefore = globalAnimatable.getCurrentArrangement();

    while (delta >= this->timeLeft) {
        globalAnimatable.Update();

        delta -= this->timeLeft;
        this->timeLeft = 1.f / frameRate;

        if (!globalAnimatable.getAnimating() && this->looping) {
            globalAnimatable.setAnimationFromIndex(globalAnimatable.getCurrentAnimationIndex());
            globalAnimatable.setAnimating(true);
        }

        this->playing = globalAnimatable.getAnimating();
    }

    const auto* arrangeAfter = globalAnimatable.getCurrentArrangement();

    if (arrangeBefore != arrangeAfter) {
        for (auto& part : appState.selectedParts) {
            int p = appState.getMatchingNamePartIndex(
                arrangeBefore->parts.at(part.index), *arrangeAfter
            );

            if (p >= 0)
                part.index = p;
        }

        appState.correctSelectedParts();
    }

    this->timeLeft -= delta;
}
