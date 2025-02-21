#ifndef COMMANDDELETEANIMATIONKEY_HPP
#define COMMANDDELETEANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

class CommandDeleteAnimationKey : public BaseCommand {
public:
    // Constructor: Delete key by cellanimIndex, animationIndex and keyIndex.
    CommandDeleteAnimationKey(
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex)
    {
        this->key = this->getKey();
    }
    ~CommandDeleteAnimationKey() = default;

    void Execute() override {
        CellAnim::Animation& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        CellAnim::Animation& animation = this->getAnimation();

        auto it = animation.keys.begin() + this->keyIndex;
        animation.keys.insert(it, this->key);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    CellAnim::AnimationKey key;

    CellAnim::AnimationKey& getKey() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex)
            .keys.at(this->keyIndex);
    }

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDDELETEANIMATIONKEY_HPP
