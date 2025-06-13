#ifndef COMMANDMOVEANIMATIONKEY_HPP
#define COMMANDMOVEANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"

class CommandMoveAnimationKey : public BaseCommand {
public:
    // Constructor: Move key by cellanimIndex, animationIndex and keyIndex.
    CommandMoveAnimationKey(
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex,
        bool moveDown, bool preserveHold
    ) :
        mMoveDown(moveDown), mPreserveHold(preserveHold),
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex), mKeyIndex(keyIndex)
    {}
    ~CommandMoveAnimationKey() = default;

    void Execute() override {
        CellAnim::Animation& animation = getAnimation();

        int nSwap = mKeyIndex + (mMoveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(animation.keys.size())) {
            std::swap(
                animation.keys.at(mKeyIndex),
                animation.keys.at(nSwap)
            );

            if (mPreserveHold)
                std::swap(
                    animation.keys.at(mKeyIndex).holdFrames,
                    animation.keys.at(nSwap).holdFrames
                );
        }

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        CellAnim::Animation& animation = getAnimation();

        int nSwap = mKeyIndex + (mMoveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(animation.keys.size())) {
            std::swap(
                animation.keys.at(mKeyIndex),
                animation.keys.at(nSwap)
            );

            if (mPreserveHold)
                std::swap(
                    animation.keys.at(mKeyIndex).holdFrames,
                    animation.keys.at(nSwap).holdFrames
                );
        }

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;
    unsigned mKeyIndex;

    bool mMoveDown;

    bool mPreserveHold;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDMOVEANIMATIONKEY_HPP
