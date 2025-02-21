#ifndef PLAYERMANAGER_HPP
#define PLAYERMANAGER_HPP

#include "Singleton.hpp"

#include <algorithm>

#include <chrono>

#include <memory>

#include "anim/RvlCellAnim.hpp"

#include "SessionManager.hpp"

class PlayerManager : public Singleton<PlayerManager> {
    friend class Singleton<PlayerManager>; // Allow access to base class constructor

private:
    int holdFramesLeft { 0 };

    unsigned animationIndex { 0 };
    unsigned keyIndex { 0 };

    std::chrono::steady_clock::time_point tickPrev;
    float timeLeft { 0.f };
public:
    bool playing { false };

    bool looping { false };

    unsigned frameRate { 60 };

public:
    void Update();

    void ResetTimer();

    unsigned getAnimationIndex() const { return this->animationIndex; }
    void setAnimationIndex(unsigned index);

    RvlCellAnim::Animation& getAnimation() const {
        return this->getCellanim()->animations.at(this->animationIndex);
    }

    unsigned getKeyIndex() const { return this->keyIndex; }
    void setKeyIndex(unsigned index);

    RvlCellAnim::AnimationKey& getKey() const {
        return this->getAnimation().keys.at(this->keyIndex);
    }

    unsigned getArrangementIndex() const {
        return this->getKey().arrangementIndex;
    }

    RvlCellAnim::Arrangement& getArrangement() const {
        return this->getCellanim()->arrangements.at(this->getArrangementIndex());
    }

    unsigned getKeyCount() const {
        return this->getAnimation().keys.size();
    }
    int getHoldFramesLeft() const { return this->holdFramesLeft; }

    bool getPlaying() const { return this->playing; }
    void setPlaying(bool animating);

    unsigned getTotalFrames() const;
    unsigned getElapsedFrames() const;

    // 0.f - 1.f
    float getAnimationProgression() const {
        return
            static_cast<float>(this->getTotalFrames()) / this->getElapsedFrames();
    }

    // After for example a session switch or another change that can reduce the amount
    // of animations, keys, or parts, the state needs to be 'corrected' to align with
    // the new circumstances.
    void correctState() {
        unsigned animCount = this->getCellanim()->animations.size();
        unsigned animIndex = std::min(
            animCount - 1,
            this->animationIndex
        );

        // setAnimationIndex clamps the key count & corrects the part selection.
        this->setAnimationIndex(animIndex);
    }

private:
    const std::shared_ptr<RvlCellAnim::RvlCellAnimObject>& getCellanim() const {
        return SessionManager::getInstance().getCurrentSession()
            ->getCurrentCellanim().object;
    }

    PlayerManager() {} // Private constructor to prevent instantiation
};

#endif // PLAYERMANAGER_HPP
