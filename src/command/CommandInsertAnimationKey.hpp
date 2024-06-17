#ifndef COMMANDINSERTANIMATIONKEY_HPP
#define COMMANDINSERTANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

#include <algorithm>

class CommandInsertAnimationKey : public BaseCommand {
public:
    // Constructor: Insert key by cellanimIndex, animationIndex, and keyIndex from key.
    CommandInsertAnimationKey(
        uint16_t cellanimIndex,
        uint16_t animationIndex,
        uint32_t keyIndex,
        RvlCellAnim::AnimationKey key
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex),
        key(key)
    {}

    void Execute() override {
        auto& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.insert(it, this->key);

        GET_APP_STATE;
        appState.playerState.currentFrame = std::clamp<uint16_t>(
            appState.playerState.currentFrame,
            0,
            static_cast<uint16_t>(appState.globalAnimatable->getCurrentAnimation()->keys.size() - 1)
        );
        appState.playerState.updateSetFrameCount();
        appState.playerState.updateCurrentFrame();
    }

    void Rollback() override {
        auto& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.erase(it);

        GET_APP_STATE;
        appState.playerState.currentFrame = std::clamp<uint16_t>(
            appState.playerState.currentFrame,
            0,
            static_cast<uint16_t>(appState.globalAnimatable->getCurrentAnimation()->keys.size() - 1)
        );
        appState.playerState.updateSetFrameCount();
        appState.playerState.updateCurrentFrame();
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;
    uint32_t keyIndex;

    RvlCellAnim::AnimationKey key;

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex)
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDINSERTANIMATIONKEY_HPP