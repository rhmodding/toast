#ifndef COMMANDMODIFYANIMATION_HPP
#define COMMANDMODIFYANIMATION_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"


class CommandModifyAnimation : public BaseCommand {
public:
    // Constructor: Replace animation from cellanimIndex and animationIndex by newAnimation.
    CommandModifyAnimation(
        unsigned cellanimIndex, unsigned animationIndex,
        CellAnim::Animation newAnimation
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex),
        mNewAnimation(newAnimation)
    {
        mOldAnimation = getAnimation();
    }
    ~CommandModifyAnimation() = default;

    void Execute() override {
        getAnimation() = mNewAnimation;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        getAnimation() = mOldAnimation;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;

    CellAnim::Animation mOldAnimation;
    CellAnim::Animation mNewAnimation;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDMODIFYANIMATION_HPP
