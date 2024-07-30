#ifndef PLAYERMANAGER_HPP
#define PLAYERMANAGER_HPP

#include "Singleton.hpp"

#include "AppState.hpp"

#include <algorithm>

// Stores instance of PlayerManager in local playerManager.
#define GET_PLAYER_MANAGER PlayerManager& playerManager = PlayerManager::getInstance()

class PlayerManager : public Singleton<PlayerManager> {
    friend class Singleton<PlayerManager>; // Allow access to base class constructor

private:
    std::chrono::steady_clock::time_point previous;
    float timeLeft{ 0.f };
public:
    bool playing{ false };

    bool looping{ false };

    uint32_t currentFrame{ 0 };

    uint16_t frameRate{ 60 };

public:
    void Update();

    void ResetTimer();

    inline uint16_t getCurrentKeyIndex() {
        return AppState::getInstance().globalAnimatable->getCurrentKeyIndex();
    }
    inline uint16_t getKeyCount() {
        return AppState::getInstance().globalAnimatable->getCurrentAnimation()->keys.size();
    }
    inline int32_t getHoldFramesLeft() {
        return AppState::getInstance().globalAnimatable->getHoldFramesLeft();
    }

    void setCurrentKeyIndex(uint16_t index);

    void setAnimating(bool animating);

    uint16_t getTotalPseudoFrames();
    uint16_t getCurrentPseudoFrames();

    inline float getAnimationProgression() {
        return this->getCurrentPseudoFrames() / static_cast<float>(this->getTotalPseudoFrames());
    }

    inline void clampCurrentKeyIndex() {
        this->setCurrentKeyIndex(std::clamp<uint16_t>(
            this->getCurrentKeyIndex(),
            0,
            AppState::getInstance().globalAnimatable->getCurrentAnimation()->keys.size() - 1
        ));
    }

private:
    PlayerManager() {} // Private constructor to prevent instantiation
};

#endif // PLAYERMANAGER_HPP
