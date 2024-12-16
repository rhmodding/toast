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
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex,
        bool moveDown
    ) :
        moveDown(moveDown),
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex)
    {}
    ~CommandMoveArrangementPart() = default;

    void Execute() override {
        GET_APP_STATE;

        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        int nSwap = this->partIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < (int)arrangement.parts.size())
            std::swap(
                arrangement.parts.at(this->partIndex),
                arrangement.parts.at(nSwap)
            );

        if (appState.isPartSelected(this->partIndex)) {
            appState.setPartSelected(this->partIndex, false);
            appState.setPartSelected(nSwap, true);
        }
        else if (appState.isPartSelected(nSwap)) {
            appState.setPartSelected(nSwap, false);
            appState.setPartSelected(this->partIndex, true);
        }

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        // Re-swap
        this->Execute();
    }

private:
    bool moveDown;

    unsigned cellanimIndex;
    unsigned arrangementIndex;
    unsigned partIndex;

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
