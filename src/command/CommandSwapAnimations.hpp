#ifndef COMMANDSWAPANIMATIONS_HPP
#define COMMANDSWAPANIMATIONS_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandSwapAnimations : public BaseCommand {
public:
    // Constructor: Swap animations by cellanimIndex, animationIndexA and animationIndexB.
    CommandSwapAnimations(
        uint16_t cellanimIndex,
        uint16_t animationIndexA, uint16_t animationIndexB,
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
    uint16_t cellanimIndex;
    uint16_t animationIndexA;
    uint16_t animationIndexB;

    bool swapNames;

    void updateAnimationState() {
        AppState& appState = AppState::getInstance();

        unsigned newSelectedAnimation { 0 };
        bool selectedDifferentAnimation { false };

        if (
            appState.selectedAnimation == animationIndexA ||
            appState.selectedAnimation == animationIndexB
        ) {
            newSelectedAnimation =
                appState.selectedAnimation == animationIndexA ? animationIndexB :
                appState.selectedAnimation == animationIndexB ? animationIndexA :
                appState.selectedAnimation;

            selectedDifferentAnimation = newSelectedAnimation != appState.selectedAnimation;

            appState.selectedAnimation = newSelectedAnimation;

            appState.globalAnimatable.setAnimationFromIndex(appState.selectedAnimation);

            if (selectedDifferentAnimation)
                PlayerManager::getInstance().setCurrentKeyIndex(0);

            appState.correctSelectedParts();
        }
    }

    std::vector<RvlCellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations;
    }
};

#endif // COMMANDSWAPANIMATIONS_HPP
