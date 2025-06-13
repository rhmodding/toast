#ifndef COMMANDDELETEARRANGEMENTPART_HPP
#define COMMANDDELETEARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"

class CommandDeleteArrangementPart : public BaseCommand {
public:
    // Constructor: Delete part by cellanimIndex, arrangementIndex and partIndex.
    CommandDeleteArrangementPart(
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex
    ) :
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex), mPartIndex(partIndex)
    {
        mPart = getPart();
    }
    ~CommandDeleteArrangementPart() = default;

    void Execute() override {
        CellAnim::Arrangement& arrangement = getArrangement();

        auto it = arrangement.parts.begin() + mPartIndex;
        arrangement.parts.erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        CellAnim::Arrangement& arrangement = getArrangement();

        auto it = arrangement.parts.begin() + mPartIndex;
        arrangement.parts.insert(it, mPart);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mArrangementIndex;
    unsigned mPartIndex;

    CellAnim::ArrangementPart mPart;

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

#endif // COMMANDDELETEARRANGEMENTPART_HPP
