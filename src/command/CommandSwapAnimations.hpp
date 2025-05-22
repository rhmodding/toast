#ifndef COMMANDSWAPANIMATIONS_HPP
#define COMMANDSWAPANIMATIONS_HPP

#include "BaseCommand.hpp"

#include "../cellanim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandSwapAnimations : public BaseCommand {
public:
    // Constructor: Swap animations by cellanimIndex, animationIndexA and animationIndexB.
    CommandSwapAnimations(
        unsigned cellanimIndex,
        unsigned animationIndexA, unsigned animationIndexB,
        bool swapNames
    ) :
        mCellAnimIndex(cellanimIndex),
        mAnimationIndexA(animationIndexA), mAnimationIndexB(animationIndexB),
        mSwapNames(swapNames)
    {}
    ~CommandSwapAnimations() = default;

    void Execute() override {
        std::swap(
            getAnimations().at(mAnimationIndexA), getAnimations().at(mAnimationIndexB)
        );

        if (!mSwapNames) {
            std::swap(
                getAnimations().at(mAnimationIndexA).name, getAnimations().at(mAnimationIndexB).name
            );
        }

        updateAnimationState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        // Re-swap
        Execute();
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndexA;
    unsigned mAnimationIndexB;

    bool mSwapNames;

    void updateAnimationState() {
        PlayerManager& playerManager = PlayerManager::getInstance();

        unsigned newSelectedAnimation { 0 };
        bool selectedDifferentAnimation { false };

        unsigned currentAnimation = playerManager.getAnimationIndex();

        if (
            currentAnimation == mAnimationIndexA ||
            currentAnimation == mAnimationIndexB
        ) {
            newSelectedAnimation =
                currentAnimation == mAnimationIndexA ? mAnimationIndexB :
                currentAnimation == mAnimationIndexB ? mAnimationIndexA :
                currentAnimation;

            selectedDifferentAnimation = newSelectedAnimation != currentAnimation;

            playerManager.setAnimationIndex(newSelectedAnimation);
            if (selectedDifferentAnimation)
                playerManager.setKeyIndex(0);

            PlayerManager::getInstance().correctState();
        }
    }

    std::vector<CellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimations();
    }
};

#endif // COMMANDSWAPANIMATIONS_HPP
