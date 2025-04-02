#ifndef COMMANDMOVEARRANGEMENTPART_HPP
#define COMMANDMOVEARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "../cellanim/CellAnim.hpp"

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
        SessionManager& sessionManager = SessionManager::getInstance();

        CellAnim::Arrangement& arrangement = this->getArrangement();

        int nSwap = this->partIndex + (moveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(arrangement.parts.size()))
            std::swap(
                arrangement.parts.at(this->partIndex),
                arrangement.parts.at(nSwap)
            );

        auto& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

        if (selectionState.isPartSelected(this->partIndex)) {
            selectionState.setPartSelected(this->partIndex, false);
            selectionState.setPartSelected(nSwap, true);
        }
        else if (selectionState.isPartSelected(nSwap)) {
            selectionState.setPartSelected(nSwap, false);
            selectionState.setPartSelected(this->partIndex, true);
        }

        sessionManager.setCurrentSessionModified(true);
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

    CellAnim::ArrangementPart& getPart() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex)
            .parts.at(this->partIndex);
    }

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDMOVEARRANGEMENTPART_HPP
