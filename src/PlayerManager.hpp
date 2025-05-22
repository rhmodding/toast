#ifndef PLAYERMANAGER_HPP
#define PLAYERMANAGER_HPP

#include "Singleton.hpp"

#include <algorithm>

#include <chrono>

#include <memory>

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
        return this->getAnimation().keys.at(mKeyIndex);
    }

    unsigned getArrangementIndex() const {
        return this->getKey().arrangementIndex;
    }

    CellAnim::Arrangement& getArrangement() const {
        return this->getCellAnim()->getArrangement(this->getArrangementIndex());
    }

    unsigned getKeyCount() const {
        return this->getAnimation().keys.size();
    }
    int getHoldFramesLeft() const { return mHoldFramesLeft; }

    bool getPlaying() const { return mPlaying; }
    void setPlaying(bool playing);

    unsigned getTotalFrames() const;
    unsigned getElapsedFrames() const;

    // 0.f - 1.f
    float getAnimationProgression() const {
        return static_cast<float>(this->getTotalFrames()) / this->getElapsedFrames();
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

    int mHoldFramesLeft { 0 };

    unsigned mAnimationIndex { 0 };
    unsigned mKeyIndex { 0 };

    std::chrono::steady_clock::time_point mTickPrev;
    float mTimeLeft { 0.f };

    OnionSkinState mOnionSkinState;

public: // TODO: hack; this should be private .. make accessor methods
    bool mLooping { false };

    unsigned mFrameRate { 60 };
};

#endif // PLAYERMANAGER_HPP
