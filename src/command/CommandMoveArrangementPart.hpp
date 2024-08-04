#ifndef COMMANDMOVEARRANGEMENTPART_HPP
#define COMMANDMOVEARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandMoveArrangementPart : public BaseCommand {
public:
    // Constructor: Move part by cellanimIndex, arrangementIndex and partIndex.
    CommandMoveArrangementPart(
        uint16_t cellanimIndex, uint32_t arrangementIndex, uint32_t partIndex,
        bool moveDown
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex),
        moveDown(moveDown)
    {}

    void Execute() override {
        GET_APP_STATE;

        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        // An integer is used since nSwap can be negative.
        int32_t nSwap = this->partIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < arrangement.parts.size())
            std::swap(
                arrangement.parts.at(this->partIndex),
                arrangement.parts.at(nSwap)
            );

        if (appState.selectedPart == this->partIndex)
            appState.selectedPart = nSwap;
    }

    void Rollback() override {
        GET_APP_STATE;

        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        int32_t nSwap = this->partIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < arrangement.parts.size())
            std::swap(
                arrangement.parts.at(this->partIndex),
                arrangement.parts.at(nSwap)
            );

        if (appState.selectedPart == nSwap)
            appState.selectedPart = this->partIndex;
    }

private:
    bool moveDown;

    uint16_t cellanimIndex;
    uint32_t arrangementIndex;
    uint32_t partIndex;

    RvlCellAnim::ArrangementPart& getPart() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex)
            .parts.at(this->partIndex);
    }

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDMOVEARRANGEMENTPART_HPP
