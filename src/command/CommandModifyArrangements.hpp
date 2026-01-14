#ifndef COMMANDMODIFYARRANGEMENTS_HPP
#define COMMANDMODIFYARRANGEMENTS_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"


class CommandModifyArrangements : public BaseCommand {
public:
    // Constructor: Replace arrangements by cellanimIndex by newArrangements.
    CommandModifyArrangements(
        unsigned cellanimIndex,
        std::vector<CellAnim::Arrangement> newArrangements
    ) :
        mCellAnimIndex(cellanimIndex),
        mNewArrangements(newArrangements)
    {
        mOldArrangements = getArrangements();
    }
    ~CommandModifyArrangements() = default;

    void execute() override {
        getArrangements() = mNewArrangements;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void rollback() override {
        getArrangements() = mOldArrangements;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;

    std::vector<CellAnim::Arrangement> mOldArrangements;
    std::vector<CellAnim::Arrangement> mNewArrangements;

    std::vector<CellAnim::Arrangement>& getArrangements() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangements();
    }
};

#endif // COMMANDMODIFYARRANGEMENTS_HPP
