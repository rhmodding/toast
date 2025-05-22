#ifndef COMMANDMODIFYANIMATIONS_HPP
#define COMMANDMODIFYANIMATIONS_HPP

#include "BaseCommand.hpp"

#include "../cellanim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

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

    void Execute() override {
        getAnimations() = mNewAnimations;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        getAnimations() = mOldAnimations;

        PlayerManager::getInstance().correctState();

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
