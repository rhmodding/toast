#ifndef COMMANDINSERTANIMATIONKEY_HPP
#define COMMANDINSERTANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "manager/SessionManager.hpp"

#include "cellanim/CellAnim.hpp"

class CommandInsertAnimationKey : public BaseCommand {
public:
    // Constructor: Insert key by cellanimIndex, animationIndex, and keyIndex from key.
    CommandInsertAnimationKey(
        unsigned cellanimIndex,
        unsigned animationIndex,
        unsigned keyIndex,
        CellAnim::AnimationKey key
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex), mKeyIndex(keyIndex),
        mKey(key)
    {}
    ~CommandInsertAnimationKey() = default;

    void Execute() override {
        auto& animation = getAnimation();

        auto it = animation.keys.begin() + mKeyIndex;
        animation.keys.insert(it, mKey);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& animation = getAnimation();

        auto it = animation.keys.begin() + mKeyIndex;
        animation.keys.erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;
    unsigned mKeyIndex;

    CellAnim::AnimationKey mKey;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDINSERTANIMATIONKEY_HPP
