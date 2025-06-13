#ifndef COMMANDMODIFYANIMATIONKEY_HPP
#define COMMANDMODIFYANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

class CommandModifyAnimationKey : public BaseCommand {
public:
    // Constructor: Replace key from cellanimIndex, animationIndex and keyIndex by newKey.
    CommandModifyAnimationKey(
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex,
        CellAnim::AnimationKey newKey
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex), mKeyIndex(keyIndex),
        mNewKey(newKey)
    {
        mOldKey = getKey();
    }
    ~CommandModifyAnimationKey() = default;

    void Execute() override {
        getKey() = mNewKey;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        getKey() = mOldKey;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;
    unsigned mKeyIndex;

    CellAnim::AnimationKey mOldKey;
    CellAnim::AnimationKey mNewKey;

    CellAnim::AnimationKey& getKey() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex)
            .keys.at(mKeyIndex);
    }

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangement(getKey().arrangementIndex);
    }
};

#endif // COMMANDMODIFYANIMATIONKEY_HPP
