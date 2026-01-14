#ifndef COMMANDMODIFYANIMATION_HPP
#define COMMANDMODIFYANIMATION_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

#include "Logging.hpp"

class CommandModifyAnimation : public BaseCommand {
public:
    // Constructor: Replace animation from cellanimIndex and animationIndex by newAnimation.
    CommandModifyAnimation(
        unsigned cellanimIndex, unsigned animationIndex,
        CellAnim::Animation newAnimation
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex),
        mNewAnimation(newAnimation),
        mIsError(false)
    {
        if (mNewAnimation.keys.empty()) {
            Logging::error("[CommmandModifyAnimation] Cannot submit animation with no keys: it's super illegal!!!");
            mIsError = true;
            return;
        }

        mOldAnimation = getAnimation();
    }
    ~CommandModifyAnimation() = default;

    void execute() override {
        if (mIsError) {
            return;
        }

        getAnimation() = mNewAnimation;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void rollback() override {
        if (mIsError) {
            return;
        }

        getAnimation() = mOldAnimation;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;

    CellAnim::Animation mOldAnimation;
    CellAnim::Animation mNewAnimation;

    bool mIsError;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDMODIFYANIMATION_HPP
