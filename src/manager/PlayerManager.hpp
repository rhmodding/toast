#ifndef PLAYER_MANAGER_HPP
#define PLAYER_MANAGER_HPP

#include "Singleton.hpp"

#include <algorithm>

#include <chrono>

#include <memory>

#include <limits>

#include "OnionSkinState.hpp"

#include "cellanim/CellAnim.hpp"

#include "SessionManager.hpp"

class PlayerManager : public Singleton<PlayerManager> {
    friend class Singleton<PlayerManager>;

private:
    PlayerManager() = default;
public:
    ~PlayerManager() = default;

public:
    void Update();

    void ResetTimer();

    unsigned getAnimationIndex() const { return mAnimationIndex; }
    void setAnimationIndex(unsigned index);

    CellAnim::Animation& getAnimation() const;

    unsigned getKeyIndex() const { return mKeyIndex; }
    void setKeyIndex(unsigned index);

    CellAnim::AnimationKey& getKey() const {
        return getAnimation().keys.at(mKeyIndex);
    }

    unsigned getArrangementIndex() const {
        return getKey().arrangementIndex;
    }

    CellAnim::Arrangement& getArrangement() const {
        return getCellAnim()->getArrangement(getArrangementIndex());
    }

    unsigned getKeyCount() const {
        return getAnimation().keys.size();
    }
    int getHoldFramesLeft() const { return mHoldFramesLeft; }

    bool getPlaying() const { return mPlaying; }
    void setPlaying(bool playing) { mPlaying = playing; }

    bool getLooping() const { return mLooping; }
    void setLooping(bool looping) { mLooping = looping; }

    unsigned getFrameRate() const { return mFrameRate; }
    void setFrameRate(unsigned frameRate) { mFrameRate = frameRate; }

    unsigned getTotalFrames() const;
    unsigned getElapsedFrames() const;

    // 0.f - 1.f
    float getAnimationProgression() const {
        return static_cast<float>(getTotalFrames()) / getElapsedFrames();
    }

    // After for example a session switch or another change that can reduce the amount
    // of animations, keys, or parts, the state needs to be 'corrected' to align with
    // the new circumstances.
    void correctState();

    unsigned getArrangementModeIdx() const;
    void setArrangementModeIdx(unsigned index);

    OnionSkinState& getOnionSkinState() { return mOnionSkinState; }

private:
    const std::shared_ptr<CellAnim::CellAnimObject>& getCellAnim() const {
        return SessionManager::getInstance().getCurrentSession()
            ->getCurrentCellAnim().object;
    }

    bool arrangementModeEnabled() const {
        return SessionManager::getInstance().getCurrentSession()->arrangementMode;
    }

private:
    bool mPlaying { false };
    bool mLooping { false };

    unsigned mFrameRate { 60 };

    int mHoldFramesLeft { std::numeric_limits<int>::max() };

    unsigned mAnimationIndex { 0 };
    unsigned mKeyIndex { 0 };

    std::chrono::steady_clock::time_point mTickPrev;
    float mTimeLeft { 0.f };

    OnionSkinState mOnionSkinState;
};

#endif // PLAYER_MANAGER_HPP
