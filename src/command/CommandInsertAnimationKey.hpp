#ifndef COMMANDINSERTANIMATIONKEY_HPP
#define COMMANDINSERTANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../AppState.hpp"

#include "../SessionManager.hpp"

#include "../cellanim/CellAnim.hpp"

class CommandInsertAnimationKey : public BaseCommand {
public:
    // Constructor: Insert key by cellanimIndex, animationIndex, and keyIndex from key.
    CommandInsertAnimationKey(
        unsigned cellanimIndex,
        unsigned animationIndex,
        unsigned keyIndex,
        CellAnim::AnimationKey key
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex),
        key(key)
    {}
    ~CommandInsertAnimationKey() = default;

    void Execute() override {
        auto& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.insert(it, this->key);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    CellAnim::AnimationKey key;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDINSERTANIMATIONKEY_HPP
