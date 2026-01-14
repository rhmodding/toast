#ifndef COMMANDDELETEANIMATION_HPP
#define COMMANDDELETEANIMATION_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

class CommandDeleteAnimation : public BaseCommand {
public:
    // Constructor: Delete animation by cellanimIndex and animationIndex.
    CommandDeleteAnimation(
        unsigned cellanimIndex, unsigned animationIndex
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex)
    {
        mAnimation = getAnimation();
    }
    ~CommandDeleteAnimation() = default;

    void execute() override {
        auto it = getAnimations().begin() + mAnimationIndex;
        getAnimations().erase(it);

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void rollback() override {
        auto it = getAnimations().begin() + mAnimationIndex;
        getAnimations().insert(it, mAnimation);

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;

    CellAnim::Animation mAnimation;

    std::vector<CellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimations();
    }

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDDELETEANIMATION_HPP
