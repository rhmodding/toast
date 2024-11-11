#ifndef PLAYERMANAGER_HPP
#define PLAYERMANAGER_HPP

#include "Singleton.hpp"

#include <algorithm>

#include "AppState.hpp"

// Stores instance of PlayerManager in local playerManager.
#define GET_PLAYER_MANAGER PlayerManager& playerManager = PlayerManager::getInstance()

class PlayerManager : public Singleton<PlayerManager> {
    friend class Singleton<PlayerManager>; // Allow access to base class constructor

private:
    std::chrono::steady_clock::time_point previous;
    float timeLeft { 0.f };
public:
    bool playing { false };

    bool looping { false };

    unsigned currentFrame { 0 };

    unsigned frameRate { 60 };

public:
    void Update();

    void ResetTimer();

    unsigned getCurrentKeyIndex() {
        return AppState::getInstance().globalAnimatable->
            getCurrentKeyIndex();
    }
    unsigned getKeyCount() {
        return AppState::getInstance().globalAnimatable->
            getCurrentAnimation()->keys.size();
    }
    
    int getHoldFramesLeft() {
        return AppState::getInstance().globalAnimatable->
            getHoldFramesLeft();
    }

    void setCurrentKeyIndex(unsigned index);
    void setPlaying(bool animating);

    // Pseudoframes are animation keys + their hold time.
    unsigned getTotalPseudoFrames();
    unsigned getElapsedPseudoFrames();

    // 0.f - 1.f
    float getAnimationProgression() {
        return
            this->getElapsedPseudoFrames() /
            static_cast<float>(this->getTotalPseudoFrames());
    }

    void clampCurrentKeyIndex() {
        this->setCurrentKeyIndex(std::min<unsigned>(
            this->getCurrentKeyIndex(),
            AppState::getInstance().globalAnimatable->getCurrentAnimation()->keys.size() - 1
        ));
    }

private:
    PlayerManager() {} // Private constructor to prevent instantiation
};

#endif // PLAYERMANAGER_HPP
