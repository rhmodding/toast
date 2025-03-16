#ifndef COMMANDMOVEANIMATIONKEY_HPP
#define COMMANDMOVEANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "../cellanim/CellAnim.hpp"

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
        CellAnim::Animation& animation = this->getAnimation();

        // An signed int is used since nSwap can be negative.
        int nSwap = this->keyIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(animation.keys.size())) {
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

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        CellAnim::Animation& animation = this->getAnimation();

        int nSwap = this->keyIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(animation.keys.size())) {
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

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    bool moveDown;

    bool preserveHold;

    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDMOVEANIMATIONKEY_HPP
