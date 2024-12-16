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
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex,
        bool moveDown, bool preserveHold
    ) :
        moveDown(moveDown), preserveHold(preserveHold),
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex)
    {}
    ~CommandMoveAnimationKey() = default;

    void Execute() override {
        RvlCellAnim::Animation& animation = this->getAnimation();

        // An signed int is used since nSwap can be negative.
        int nSwap = this->keyIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < (int)animation.keys.size()) {
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

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        RvlCellAnim::Animation& animation = this->getAnimation();

        int nSwap = this->keyIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < (int)animation.keys.size()) {
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

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    bool moveDown;

    bool preserveHold;

    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDMOVEANIMATIONKEY_HPP
