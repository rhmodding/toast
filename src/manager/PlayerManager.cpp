#include "cellanim/CellAnim.hpp"

#include <numeric>

#include "PlayerManager.hpp"

static CellAnim::Animation arrangementModeAnim {
    .keys = std::vector<CellAnim::AnimationKey>({
        CellAnim::AnimationKey {}
    })
};

static void matchSelectedParts(const CellAnim::Arrangement& before, const CellAnim::Arrangement& after) {
    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    for (auto& part : selectionState.mSelectedParts) {
        int p = selectionState.getMatchingNamePartIndex(before.parts.at(part.index), after, part.index);
        if (p >= 0)
            part.index = p;
    }

    selectionState.correctSelectedParts();
}

void PlayerManager::Update() {
    if (!mPlaying || arrangementModeEnabled())
        return;

    auto tickNow = std::chrono::steady_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::duration<float>>(tickNow - mTickPrev).count();

    mTickPrev = tickNow;

    const auto& keys = getCellAnim()->getAnimation(mAnimationIndex).keys;
    const auto& arrangements = getCellAnim()->getArrangements();

    unsigned arrangementIdxBefore = keys.at(mKeyIndex).arrangementIndex;

    const auto& currentAnimation = getCellAnim()->getAnimation(mAnimationIndex);

    while (delta >= mTimeLeft) {
        if (mHoldFramesLeft > 1) {
            mHoldFramesLeft--;

            delta -= mTimeLeft;
            mTimeLeft = 1.f / mFrameRate;
            continue;
        }

        bool foundValidKey = false;

        while (mKeyIndex + 1 < currentAnimation.keys.size()) {
            mKeyIndex++;
            if (currentAnimation.keys[mKeyIndex].holdFrames > 0) {
                foundValidKey = true;
                break;
            }
        }

        if (!foundValidKey) {
            if (!mLooping) {
                mPlaying = false;
                return;
            }

            mKeyIndex = 0;
            while (
                mKeyIndex < currentAnimation.keys.size() &&
                currentAnimation.keys[mKeyIndex].holdFrames == 0
            )
                mKeyIndex++;

            if (mKeyIndex >= currentAnimation.keys.size()) {
                mKeyIndex = currentAnimation.keys.size() - 1;
                mPlaying = false;
                return;
            }
        }

        mHoldFramesLeft = currentAnimation.keys[mKeyIndex].holdFrames;

        delta -= mTimeLeft;
        mTimeLeft = 1.f / mFrameRate;
    }

    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    if (arrangementIdxBefore != keys.at(mKeyIndex).arrangementIndex) {
        matchSelectedParts(
            arrangements.at(arrangementIdxBefore),
            arrangements.at(keys.at(mKeyIndex).arrangementIndex)
        );

        selectionState.correctSelectedParts();
    }

    mTimeLeft -= delta;
}

void PlayerManager::ResetTimer() {
    mTimeLeft = 1.f / mFrameRate;
    mTickPrev = std::chrono::steady_clock::now();
}

void PlayerManager::setAnimationIndex(unsigned index) {
    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    mAnimationIndex = index;
    mKeyIndex = std::min<unsigned>(
        mKeyIndex,
        getKeyCount() - 1
    );

    mHoldFramesLeft = std::min(
        mHoldFramesLeft, static_cast<int>(getKey().holdFrames)
    );

    selectionState.correctSelectedParts();
}

CellAnim::Animation& PlayerManager::getAnimation() const {
    if (arrangementModeEnabled())
        return arrangementModeAnim;

    return getCellAnim()->getAnimation(mAnimationIndex);
}

void PlayerManager::setKeyIndex(unsigned index) {
    const auto& keys = getAnimation().keys;
    const auto& arrangements = getCellAnim()->getArrangements();

    if (keys.at(mKeyIndex).arrangementIndex != keys.at(index).arrangementIndex) {
        matchSelectedParts(
            arrangements.at(keys.at(mKeyIndex).arrangementIndex),
            arrangements.at(keys.at(index).arrangementIndex)
        );
    }

    auto& selectionState = SessionManager::getInstance().getCurrentSession()->getCurrentSelectionState();

    mKeyIndex = index;
    mHoldFramesLeft = keys.at(index).holdFrames;

    selectionState.correctSelectedParts();
}

unsigned PlayerManager::getTotalFrames() const {
    return getAnimation().countFrames();
}

unsigned PlayerManager::getElapsedFrames() const {
    const auto& animation = getAnimation();

    /*
    unsigned totalDuration = 0;
    for (const auto& key : animation.keys)
        totalDuration += key.holdFrames;
    */

    unsigned elapsed = 0;
    for (unsigned i = 0; i < mKeyIndex; i++) {
        elapsed += animation.keys[i].holdFrames;
    }
    elapsed += (animation.keys[mKeyIndex].holdFrames - mHoldFramesLeft);

    /*
    if (elapsed >= totalDuration)
        return totalDuration - 1;
    */

    return elapsed;
}

void PlayerManager::setElapsedFrames(size_t frames) {
    const auto& animation = getAnimation();

    size_t i;
    size_t currentFrame = 0;
    for (i = 0; i < animation.keys.size(); i++) {
        size_t duration = animation.keys[i].holdFrames;
        if (frames >= currentFrame && frames < (currentFrame + duration))
            break;

        currentFrame += duration;
    }

    if (i == animation.keys.size()) {
        setKeyIndex(i - 1);
        mHoldFramesLeft = 0;
    }
    else {
        setKeyIndex(i);
        mHoldFramesLeft = animation.keys[i].holdFrames - (frames - currentFrame);
    }
}

void PlayerManager::correctState() {
    if (SessionManager::getInstance().getCurrentSessionIndex() >= 0) {
        unsigned arrangementCount = getCellAnim()->getArrangements().size();
        if (getArrangementModeIdx() >= arrangementCount)
            setArrangementModeIdx(arrangementCount - 1);

        unsigned animCount = getCellAnim()->getAnimations().size();
        unsigned animIndex = std::min(
            mAnimationIndex,
            animCount - 1
        );

        // setAnimationIndex clamps the key index & corrects the part selection.
        setAnimationIndex(animIndex);
    }
    else {
        mAnimationIndex = 0;
        mKeyIndex = 0;
    }
}

unsigned PlayerManager::getArrangementModeIdx() const {
    return arrangementModeAnim.keys[0].arrangementIndex;
}
void PlayerManager::setArrangementModeIdx(unsigned index) {
    arrangementModeAnim.keys[0].arrangementIndex = index;
}
