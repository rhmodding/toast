#ifndef COMMANDSWAPANIMATIONS_HPP
#define COMMANDSWAPANIMATIONS_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

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
        cellanimIndex(cellanimIndex),
        animationIndexA(animationIndexA), animationIndexB(animationIndexB),
        swapNames(swapNames)
    {}
    ~CommandSwapAnimations() = default;

    void Execute() override {
        std::swap(
            this->getAnimations().at(animationIndexA),
            this->getAnimations().at(animationIndexB)
        );

        if (!this->swapNames) {
            std::swap(
                this->getAnimations().at(animationIndexA).name,
                this->getAnimations().at(animationIndexB).name
            );
        }

        this->updateAnimationState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        // Re-swap
        this->Execute();
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndexA;
    unsigned animationIndexB;

    bool swapNames;

    void updateAnimationState() {
        PlayerManager& playerManager = PlayerManager::getInstance();

        unsigned newSelectedAnimation { 0 };
        bool selectedDifferentAnimation { false };

        unsigned currentAnimation = playerManager.getAnimationIndex();

        if (
            currentAnimation == animationIndexA ||
            currentAnimation == animationIndexB
        ) {
            newSelectedAnimation =
                currentAnimation == animationIndexA ? animationIndexB :
                currentAnimation == animationIndexB ? animationIndexA :
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
            ->cellanims.at(this->cellanimIndex).object
            ->animations;
    }
};

#endif // COMMANDSWAPANIMATIONS_HPP
