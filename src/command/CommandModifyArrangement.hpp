#ifndef COMMANDMODIFYARRANGEMENT_HPP
#define COMMANDMODIFYARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"


class CommandModifyArrangement : public BaseCommand {
public:
    // Constructor: Replace arrangement from cellanimIndex and arrangementIndex by newArrangement.
    CommandModifyArrangement(
        unsigned cellanimIndex, unsigned arrangementIndex,
        CellAnim::Arrangement newArrangement
    ) :
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex),
        mNewArrangement(newArrangement)
    {
        mOldArrangement = getArrangement();
    }
    ~CommandModifyArrangement() = default;

    void execute() override {
        getArrangement() = mNewArrangement;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void rollback() override {
        getArrangement() = mOldArrangement;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mArrangementIndex;

    CellAnim::Arrangement mOldArrangement;
    CellAnim::Arrangement mNewArrangement;

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangement(mArrangementIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENT_HPP
