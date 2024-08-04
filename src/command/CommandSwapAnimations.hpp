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

    void Execute() override {
        std::swap(
            this->getAnimations().at(animationIndexA),
            this->getAnimations().at(animationIndexB)
        );

        if (this->swapNames) {
            auto& map = this->getAnimationNames();

            if (map.find(animationIndexA) != map.end() && map.find(animationIndexB) != map.end()) {
                std::swap(map[animationIndexA], map[animationIndexB]);
            } else if (map.find(animationIndexA) != map.end()) {
                map[animationIndexB] = map[animationIndexA];
                map.erase(animationIndexA);
            } else if (map.find(animationIndexB) != map.end()) {
                map[animationIndexA] = map[animationIndexB];
                map.erase(animationIndexB);
            }
        }

        this->updateAnimationState();
    }

    void Rollback() override {
        this->Execute();
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndexA;
    uint16_t animationIndexB;

    bool swapNames;

    void updateAnimationState() {
        GET_APP_STATE;

        uint16_t newSelectedAnimation{ 0 };
        bool selectedDifferentAnimation{ false };

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
            appState.globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);

            if (selectedDifferentAnimation)
                PlayerManager::getInstance().setCurrentKeyIndex(0);

            appState.correctSelectedPart();
        }
    }

    std::vector<RvlCellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations;
    }

    std::unordered_map<uint16_t, std::string>& getAnimationNames() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).
            animNames;
    }
};

#endif // COMMANDSWAPANIMATIONS_HPP
