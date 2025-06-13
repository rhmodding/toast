#ifndef COMMANDINSERTARRANGEMENTPART_HPP
#define COMMANDINSERTARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"


class CommandInsertArrangementPart : public BaseCommand {
public:
    // Constructor: Insert part by cellanimIndex, arrangementIndex and partIndex from part.
    CommandInsertArrangementPart(
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex,
        CellAnim::ArrangementPart part
    ) :
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex), mPartIndex(partIndex),
        mPart(part)
    {}
    ~CommandInsertArrangementPart() = default;

    void Execute() override {
        CellAnim::Arrangement& arrangement = getArrangement();

        auto it = arrangement.parts.begin() + mPartIndex;
        arrangement.parts.insert(it, mPart);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        CellAnim::Arrangement& arrangement = getArrangement();

        auto it = arrangement.parts.begin() + mPartIndex;
        arrangement.parts.erase(it);

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
};

#endif // COMMANDINSERTARRANGEMENTPART_HPP
