#ifndef COMMANDMODIFYANIMATIONS_HPP
#define COMMANDMODIFYANIMATIONS_HPP

#include "BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

class CommandModifyAnimations : public BaseCommand {
public:
    // Constructor: Replace animations by cellanimIndex by newAnimations.
    CommandModifyAnimations(
        unsigned cellanimIndex,
        std::vector<CellAnim::Animation> newAnimations
    ) :
        mCellAnimIndex(cellanimIndex),
        mNewAnimations(newAnimations)
    {
        mOldAnimations = getAnimations();
    }
    ~CommandModifyAnimations() = default;

    void execute() override {
        getAnimations() = mNewAnimations;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void rollback() override {
        getAnimations() = mOldAnimations;

        PlayerManager::getInstance().validateState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;

    std::vector<CellAnim::Animation> mOldAnimations;
    std::vector<CellAnim::Animation> mNewAnimations;

    std::vector<CellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimations();
    }
};

#endif // COMMANDMODIFYANIMATIONS_HPP
