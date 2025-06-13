#ifndef COMMANDMODIFYARRANGEMENTPART_HPP
#define COMMANDMODIFYARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"


class CommandModifyArrangementPart : public BaseCommand {
public:
    // Constructor: Replace part from cellanimIndex, arrangementIndex and partIndex by newPart.
    CommandModifyArrangementPart(
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex,
        CellAnim::ArrangementPart newPart
    ) :
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex), mPartIndex(partIndex),
        mNewPart(newPart)
    {
        mOldPart = getPart();
    }
    ~CommandModifyArrangementPart() = default;

    void Execute() override {
        getPart() = mNewPart;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        getPart() = mOldPart;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mArrangementIndex;
    unsigned mPartIndex;

    CellAnim::ArrangementPart mOldPart;
    CellAnim::ArrangementPart mNewPart;

    CellAnim::ArrangementPart& getPart() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangement(mArrangementIndex)
            .parts.at(mPartIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENTPART_HPP
