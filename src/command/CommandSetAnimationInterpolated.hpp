#ifndef COMMANDSETANIMATIONINTERPOLATED_HPP
#define COMMANDSETANIMATIONINTERPOLATED_HPP

#include "BaseCommand.hpp"

#include "manager/SessionManager.hpp"

class CommandSetAnimationInterpolated : public BaseCommand {
public:
    // Constructor: Set animation interpolation from cellanimIndex, animationIndex and isInterpolated.
    CommandSetAnimationInterpolated(
        unsigned cellanimIndex, unsigned animationIndex,
        bool isInterpolated
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex),
        mNewIsInterpolated(isInterpolated)
    {
        mOldIsInterpolated = getAnimation().isInterpolated;
    }
    ~CommandSetAnimationInterpolated() = default;

    void Execute() override {
        getAnimation().isInterpolated = mNewIsInterpolated;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        getAnimation().isInterpolated = mOldIsInterpolated;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;

    bool mOldIsInterpolated, mNewIsInterpolated;

    CellAnim::Animation& getAnimation() {
        return SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDSETANIMATIONINTERPOLATED_HPP
