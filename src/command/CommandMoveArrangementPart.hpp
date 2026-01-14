#ifndef COMMANDMOVEARRANGEMENTPART_HPP
#define COMMANDMOVEARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"

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

    void execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        CellAnim::Arrangement& arrangement = getArrangement();

        int nSwap = mPartIndex + (mMoveDown ? -1 : 1);
        if (nSwap >= 0 && nSwap < static_cast<int>(arrangement.parts.size()))
            std::swap(
                arrangement.parts.at(mPartIndex),
                arrangement.parts.at(nSwap)
            );

        auto& selectionState = sessionManager.getCurrentSession()->getPartSelectState();

        if (selectionState.checkSelected(mPartIndex)) {
            selectionState.setSelected(mPartIndex, false);
            selectionState.setSelected(nSwap, true);
        }
        else if (selectionState.checkSelected(nSwap)) {
            selectionState.setSelected(nSwap, false);
            selectionState.setSelected(mPartIndex, true);
        }

        sessionManager.setCurrentSessionModified(true);
    }

    void rollback() override {
        // Re-swap
        execute();
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
