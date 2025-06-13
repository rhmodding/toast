#ifndef COMMANDDELETEANIMATIONKEY_HPP
#define COMMANDDELETEANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

class CommandDeleteAnimationKey : public BaseCommand {
public:
    // Constructor: Delete key by cellanimIndex, animationIndex and keyIndex.
    CommandDeleteAnimationKey(
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex), mKeyIndex(keyIndex)
    {
        mKey = getKey();
    }
    ~CommandDeleteAnimationKey() = default;

    void Execute() override {
        CellAnim::Animation& animation = getAnimation();

        auto it = animation.keys.begin() + mKeyIndex;
        animation.keys.erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        CellAnim::Animation& animation = getAnimation();

        auto it = animation.keys.begin() + mKeyIndex;
        animation.keys.insert(it, mKey);

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

    CellAnim::AnimationKey& getKey() {
        return getAnimation().keys.at(mKeyIndex);
    }
};

#endif // COMMANDDELETEANIMATIONKEY_HPP
