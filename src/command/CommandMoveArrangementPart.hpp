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
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex), mPartIndex(partIndex),
        mMoveDown(moveDown)
    {}
    ~CommandMoveArrangementPart() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        CellAnim::Arrangement& arrangement = getArrangement();

        int nSwap = mPartIndex + (mMoveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(arrangement.parts.size()))
            std::swap(
                arrangement.parts.at(mPartIndex),
                arrangement.parts.at(nSwap)
            );

        auto& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

        if (selectionState.isPartSelected(mPartIndex)) {
            selectionState.setPartSelected(mPartIndex, false);
            selectionState.setPartSelected(nSwap, true);
        }
        else if (selectionState.isPartSelected(nSwap)) {
            selectionState.setPartSelected(nSwap, false);
            selectionState.setPartSelected(mPartIndex, true);
        }

        sessionManager.setCurrentSessionModified(true);
    }

    void Rollback() override {
        // Re-swap
        Execute();
    }

private:
    unsigned mCellAnimIndex;
    unsigned mArrangementIndex;
    unsigned mPartIndex;

    bool mMoveDown;

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangement(mArrangementIndex);
    }

    CellAnim::ArrangementPart& getPart() {
        return getArrangement().parts.at(mPartIndex);
    }
};

#endif // COMMANDMOVEARRANGEMENTPART_HPP
