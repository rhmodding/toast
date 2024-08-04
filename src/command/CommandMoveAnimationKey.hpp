#ifndef COMMANDMOVEANIMATIONKEY_HPP
#define COMMANDMOVEANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandMoveAnimationKey : public BaseCommand {
public:
    // Constructor: Move key by cellanimIndex, animationIndex and keyIndex.
    CommandMoveAnimationKey(
        uint16_t cellanimIndex, uint16_t animationIndex, uint32_t keyIndex,
        bool moveDown, bool preserveHold
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex),
        moveDown(moveDown), preserveHold(preserveHold)
    {}

    void Execute() override {
        GET_APP_STATE;

        RvlCellAnim::Animation& animation = this->getAnimation();

        // An integer is used since nSwap can be negative.
        int32_t nSwap = this->keyIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < animation.keys.size()) {
            std::swap(
                animation.keys.at(this->keyIndex),
                animation.keys.at(nSwap)
            );

            if (this->preserveHold)
                std::swap(
                    animation.keys.at(this->keyIndex).holdFrames,
                    animation.keys.at(nSwap).holdFrames
                );
        }

        if (appState.selectedPart == this->keyIndex)
            appState.selectedPart = nSwap;
    }

    void Rollback() override {
        GET_APP_STATE;

        RvlCellAnim::Animation& animation = this->getAnimation();

        int32_t nSwap = this->keyIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < animation.keys.size()) {
            std::swap(
                animation.keys.at(this->keyIndex),
                animation.keys.at(nSwap)
            );

            if (this->preserveHold)
                std::swap(
                    animation.keys.at(this->keyIndex).holdFrames,
                    animation.keys.at(nSwap).holdFrames
                );
        }

        if (appState.selectedPart == nSwap)
            appState.selectedPart = this->keyIndex;
    }

private:
    bool moveDown;

    bool preserveHold;

    uint16_t cellanimIndex;
    uint16_t animationIndex;
    uint32_t keyIndex;

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDMOVEANIMATIONKEY_HPP
