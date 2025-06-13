#ifndef COMMANDINSERTARRANGEMENT_HPP
#define COMMANDINSERTARRANGEMENT_HPP

#include "BaseCommand.hpp"


#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

#include "cellanim/CellAnim.hpp"

class CommandInsertArrangement : public BaseCommand {
public:
    // Constructor: Insert arrangement by cellanimIndex and arrangementIndex from arrangement.
    CommandInsertArrangement(
        unsigned cellanimIndex,
        unsigned arrangementIndex,
        CellAnim::Arrangement arrangement
    ) :
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex),
        mArrangement(arrangement)
    {}
    ~CommandInsertArrangement() = default;

    void Execute() override {
        auto& arrangements = getCellAnim()->getArrangements();

        auto it = arrangements.begin() + mArrangementIndex;
        arrangements.insert(it, mArrangement);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& arrangements = getCellAnim()->getArrangements();

        auto it = arrangements.begin() + mArrangementIndex;
        arrangements.erase(it);

        for (auto& animation : getCellAnim()->getAnimations())
            for (auto& key : animation.keys) {
                if (key.arrangementIndex >= arrangements.size())
                    key.arrangementIndex = 0;
            }

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mArrangementIndex;

    CellAnim::Arrangement mArrangement;

    std::shared_ptr<CellAnim::CellAnimObject> getCellAnim() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object;
    }
};

#endif // COMMANDINSERTARRANGEMENT_HPP
