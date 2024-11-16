#ifndef COMMANDINSERTANIMATIONKEY_HPP
#define COMMANDINSERTANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../AppState.hpp"

#include "../SessionManager.hpp"

#include "../anim/RvlCellAnim.hpp"

class CommandInsertAnimationKey : public BaseCommand {
public:
    // Constructor: Insert key by cellanimIndex, animationIndex, and keyIndex from key.
    CommandInsertAnimationKey(
        unsigned cellanimIndex,
        unsigned animationIndex,
        unsigned keyIndex,
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

        if (!appState.getArrangementMode())
            PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        auto& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.erase(it);

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    RvlCellAnim::AnimationKey key;

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDINSERTANIMATIONKEY_HPP
