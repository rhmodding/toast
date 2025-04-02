#include "cellanim/CellAnim.hpp"

#include <numeric>

#include "PlayerManager.hpp"

#include "AppState.hpp"

static CellAnim::Animation arrangementModeAnim {
    .keys = std::vector<CellAnim::AnimationKey>({
        CellAnim::AnimationKey {}
    })
};

void PlayerManager::setArrangementModeIdx(unsigned index) {
    arrangementModeAnim.keys[0].arrangementIndex = index;
}
unsigned PlayerManager::getArrangementModeIdx() const {
    return arrangementModeAnim.keys[0].arrangementIndex;
}

CellAnim::Animation& PlayerManager::getAnimation() const {
    if (this->arrangementModeEnabled())
        return arrangementModeAnim;

    return this->getCellanim()->animations.at(this->animationIndex);
}

static void matchSelectedParts(const CellAnim::Arrangement& before, const CellAnim::Arrangement& after) {
    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    for (auto& part : selectionState.selectedParts) {
        int p = selectionState.getMatchingNamePartIndex(before.parts.at(part.index), after);
        if (p >= 0)
            part.index = p;
    }

    selectionState.correctSelectedParts();
}

void PlayerManager::setAnimationIndex(unsigned index) {
    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    this->animationIndex = index;
    this->keyIndex = std::min<unsigned>(
        this->keyIndex,
        this->getKeyCount() - 1
    );

    selectionState.correctSelectedParts();
}

void PlayerManager::setKeyIndex(unsigned index) {
    const auto& keys = this->getCellanim()->animations.at(this->animationIndex).keys;
    const auto& arrangements = this->getCellanim()->arrangements;

    if (keys.at(this->keyIndex).arrangementIndex != keys.at(index).arrangementIndex) {
        matchSelectedParts(
            arrangements.at(keys.at(this->keyIndex).arrangementIndex),
            arrangements.at(keys.at(index).arrangementIndex)
        );
    }

    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    this->keyIndex = index;
    this->holdFramesLeft = keys.at(index).holdFrames;

    selectionState.correctSelectedParts();
}

void PlayerManager::ResetTimer() {
    this->timeLeft = 1.f / this->frameRate;
    this->tickPrev = std::chrono::steady_clock::now();
}

void PlayerManager::setPlaying(bool playing) {
    this->playing = playing;
}

unsigned PlayerManager::getTotalFrames() const {
    const auto& animation = this->getCellanim()->animations.at(this->animationIndex);

    return std::accumulate(
        animation.keys.begin(), animation.keys.end(), 0u,
        [](unsigned sum, const CellAnim::AnimationKey& key) {
            return sum + key.holdFrames;
        }
    );
}

unsigned PlayerManager::getElapsedFrames() const {
    if (!this->playing && this->keyIndex == 0 && this->holdFramesLeft < 1)
        return 0;

    const auto& animation = this->getCellanim()->animations.at(this->animationIndex);

    unsigned result = std::accumulate(
        animation.keys.begin(), animation.keys.begin() + this->keyIndex, 0u,
        [](unsigned sum, const CellAnim::AnimationKey& key) {
            return sum + key.holdFrames;
        }
    );

    result += animation.keys.at(this->keyIndex).holdFrames - this->holdFramesLeft;

    return result;
}

void PlayerManager::Update() {
    if (!this->playing || this->arrangementModeEnabled())
        return;

    auto tickNow = std::chrono::steady_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::duration<float>>(tickNow - this->tickPrev).count();

    this->tickPrev = tickNow;

    const auto& keys = this->getCellanim()->animations.at(this->animationIndex).keys;
    const auto& arrangements = this->getCellanim()->arrangements;

    unsigned arrangementIdxBefore = keys.at(this->keyIndex).arrangementIndex;

    while (delta >= this->timeLeft) {
        if (this->holdFramesLeft > 0) {
            this->holdFramesLeft--;

            delta -= this->timeLeft;
            this->timeLeft = 1.f / frameRate;

            continue;
        }

        const auto& currentAnimation = this->getCellanim()->animations.at(this->animationIndex);

        // End of animation, stop or loop
        if (this->keyIndex + 1 >= currentAnimation.keys.size()) {
            if (!this->looping) {
                this->playing = false;
                break;
            }

            this->keyIndex = 0;
            this->holdFramesLeft = currentAnimation.keys[0].holdFrames - 1;

            delta -= this->timeLeft;
            this->timeLeft = 1.f / frameRate;
        }
        else {
            this->keyIndex++;
            this->holdFramesLeft = currentAnimation.keys.at(this->keyIndex).holdFrames - 1;

            delta -= this->timeLeft;
            this->timeLeft = 1.f / frameRate;
        }
    }

    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    if (arrangementIdxBefore != keys.at(this->keyIndex).arrangementIndex) {
        matchSelectedParts(
            arrangements.at(arrangementIdxBefore),
            arrangements.at(keys.at(this->keyIndex).arrangementIndex)
        );

        selectionState.correctSelectedParts();
    }

    this->timeLeft -= delta;
}
