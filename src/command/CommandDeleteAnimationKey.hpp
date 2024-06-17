#ifndef COMMANDDELETEANIMATIONKEY_HPP
#define COMMANDDELETEANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

class CommandDeleteAnimationKey : public BaseCommand {
public:
    // Constructor: Delete key by cellanimIndex, animationIndex and keyIndex.
    CommandDeleteAnimationKey(
        uint16_t cellanimIndex, uint16_t animationIndex, uint32_t keyIndex
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex)
    {
        this->key = this->getKey();
    }

    void Execute() override {
        RvlCellAnim::Animation& animation = this->getAnimation();

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

    void Rollback() override {
        RvlCellAnim::Animation& animation = this->getAnimation();

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

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;
    uint32_t keyIndex;

    RvlCellAnim::AnimationKey key;

    RvlCellAnim::AnimationKey& getKey() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex)
            ->animations.at(this->animationIndex)
            .keys.at(this->keyIndex);
    }

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex)
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDDELETEANIMATIONKEY_HPP