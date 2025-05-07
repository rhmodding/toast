#include "cellanim/CellAnim.hpp"

#include <numeric>

#include "PlayerManager.hpp"

#include "AppState.hpp"

static CellAnim::Animation arrangementModeAnim {
    .keys = std::vector<CellAnim::AnimationKey>({
        CellAnim::AnimationKey {}
    })
};

static void matchSelectedParts(const CellAnim::Arrangement& before, const CellAnim::Arrangement& after) {
    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    for (auto& part : selectionState.selectedParts) {
        int p = selectionState.getMatchingNamePartIndex(before.parts.at(part.index), after);
        if (p >= 0)
            part.index = p;
    }

    selectionState.correctSelectedParts();
}

void PlayerManager::Update() {
    if (!this->playing || this->arrangementModeEnabled())
        return;

    auto tickNow = std::chrono::steady_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::duration<float>>(tickNow - this->tickPrev).count();

    this->tickPrev = tickNow;

    const auto& keys = this->getCellAnim()->animations.at(this->animationIndex).keys;
    const auto& arrangements = this->getCellAnim()->arrangements;

    unsigned arrangementIdxBefore = keys.at(this->keyIndex).arrangementIndex;

    const auto& currentAnimation = this->getCellAnim()->animations.at(this->animationIndex);

    while (delta >= this->timeLeft) {
        if (this->holdFramesLeft > 0) {
            this->holdFramesLeft--;

            delta -= this->timeLeft;
            this->timeLeft = 1.f / frameRate;
            continue;
        }

        bool foundValidKey = false;

        while (this->keyIndex + 1 < currentAnimation.keys.size()) {
            this->keyIndex++;
            if (currentAnimation.keys[this->keyIndex].holdFrames > 0) {
                foundValidKey = true;
                break;
            }
        }

        if (!foundValidKey) {
            if (!this->looping) {
                this->playing = false;
                return;
            }

            this->keyIndex = 0;
            while (
                this->keyIndex < currentAnimation.keys.size() &&
                currentAnimation.keys[this->keyIndex].holdFrames == 0
            )
                this->keyIndex++;

            if (this->keyIndex >= currentAnimation.keys.size()) {
                this->keyIndex = currentAnimation.keys.size() - 1;
                this->playing = false;
                return;
            }
        }

        this->holdFramesLeft = currentAnimation.keys[this->keyIndex].holdFrames - 1;

        delta -= this->timeLeft;
        this->timeLeft = 1.f / frameRate;
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

void PlayerManager::ResetTimer() {
    this->timeLeft = 1.f / this->frameRate;
    this->tickPrev = std::chrono::steady_clock::now();
}

void PlayerManager::setAnimationIndex(unsigned index) {
    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    this->animationIndex = index;
    this->keyIndex = std::min<unsigned>(
        this->keyIndex,
        this->getKeyCount() - 1
    );

    this->holdFramesLeft = std::min(
        this->holdFramesLeft, static_cast<int>(this->getKey().holdFrames)
    );

    selectionState.correctSelectedParts();
}

CellAnim::Animation& PlayerManager::getAnimation() const {
    if (this->arrangementModeEnabled())
        return arrangementModeAnim;

    return this->getCellAnim()->animations.at(this->animationIndex);
}

void PlayerManager::setKeyIndex(unsigned index) {
    const auto& keys = this->getCellAnim()->animations.at(this->animationIndex).keys;
    const auto& arrangements = this->getCellAnim()->arrangements;

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

void PlayerManager::setPlaying(bool playing) {
    this->playing = playing;
}

unsigned PlayerManager::getTotalFrames() const {
    const auto& animation = this->getCellAnim()->animations.at(this->animationIndex);

    unsigned totalFrames = 0;
    for (const auto& key : animation.keys) {
        totalFrames += key.holdFrames;
    }

    return totalFrames;
}

unsigned PlayerManager::getElapsedFrames() const {
    if (!this->playing && this->keyIndex == 0 && this->holdFramesLeft < 1)
        return 0;

    const auto& animation = this->getCellAnim()->animations.at(this->animationIndex);

    unsigned elapsedFrames = 0;
    for (unsigned i = 0; i < this->keyIndex + 1; i++) {
        elapsedFrames += animation.keys[i].holdFrames;
    }
    elapsedFrames -= this->holdFramesLeft;

    return elapsedFrames;
}

void PlayerManager::correctState() {
    if (SessionManager::getInstance().getCurrentSessionIndex() >= 0) {
        unsigned arrangementCount = this->getCellAnim()->arrangements.size();
        if (this->getArrangementModeIdx() >= arrangementCount)
            this->setArrangementModeIdx(arrangementCount - 1);

        unsigned animCount = this->getCellAnim()->animations.size();
        unsigned animIndex = std::min(
            this->animationIndex,
            animCount - 1
        );

        // setAnimationIndex clamps the key index & corrects the part selection.
        this->setAnimationIndex(animIndex);
    }
    else {
        this->animationIndex = 0;
        this->keyIndex = 0;
    }
}

unsigned PlayerManager::getArrangementModeIdx() const {
    return arrangementModeAnim.keys[0].arrangementIndex;
}
void PlayerManager::setArrangementModeIdx(unsigned index) {
    arrangementModeAnim.keys[0].arrangementIndex = index;
}
