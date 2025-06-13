#ifndef COMMANDDELETEARRANGEMENT_HPP
#define COMMANDDELETEARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

class CommandDeleteArrangement : public BaseCommand {
public:
    // Constructor: Delete arrangement by cellanimIndex and arrangementIndex.
    CommandDeleteArrangement(
        unsigned cellanimIndex, unsigned arrangementIndex
    ) :
        mCellAnimIndex(cellanimIndex), mArrangementIndex(arrangementIndex)
    {
        mArrangement = getArrangement();
    }
    ~CommandDeleteArrangement() = default;

    void Execute() override {
        auto& arrangements = getArrangements();

        auto it = arrangements.begin() + mArrangementIndex;
        arrangements.erase(it);

        auto& animations = SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object->getAnimations();

        for (auto& animation : animations) {
            for (auto& key : animation.keys) {
                if (key.arrangementIndex > mArrangementIndex)
                    key.arrangementIndex--;
                else if (key.arrangementIndex == mArrangementIndex)
                    key.arrangementIndex = 0;
            }
        }

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& arrangements = getArrangements();

        auto it = arrangements.begin() + mArrangementIndex;
        arrangements.insert(it, mArrangement);

        auto& animations = SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object->getAnimations();

        for (auto& animation : animations) {
            for (auto& key : animation.keys) {
                if (key.arrangementIndex >= mArrangementIndex)
                    key.arrangementIndex++;
            }
        }

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mArrangementIndex;

    CellAnim::Arrangement mArrangement;

    std::vector<CellAnim::Arrangement>& getArrangements() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangements();
    }

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getArrangement(mArrangementIndex);
    }
};

#endif // COMMANDDELETEARRANGEMENT_HPP
