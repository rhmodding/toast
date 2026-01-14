#ifndef COMMANDINSERTANIMATION_HPP
#define COMMANDINSERTANIMATION_HPP

#include "BaseCommand.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

#include "cellanim/CellAnim.hpp"

class CommandInsertAnimation : public BaseCommand {
public:
    // Constructor: Insert animation by cellanimIndex and animationIndex from animation.
    CommandInsertAnimation(
        unsigned cellanimIndex,
        unsigned animationIndex,
        CellAnim::Animation animation
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex),
        mAnimation(animation)
    {}
    ~CommandInsertAnimation() = default;

    void execute() override {
        auto& animations = getCellAnim()->getAnimations();

        auto it = animations.begin() + mAnimationIndex;
        animations.insert(it, mAnimation);

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void rollback() override {
        auto& animations = getCellAnim()->getAnimations();

        auto it = animations.begin() + mAnimationIndex;
        animations.erase(it);

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;
    CellAnim::Animation mAnimation;

    std::shared_ptr<CellAnim::CellAnimObject> getCellAnim() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object;
    }
};

#endif // COMMANDINSERTANIMATION_HPP
