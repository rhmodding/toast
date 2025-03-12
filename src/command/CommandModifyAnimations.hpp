#ifndef COMMANDMODIFYANIMATIONS_HPP
#define COMMANDMODIFYANIMATIONS_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

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
        cellanimIndex(cellanimIndex),
        newAnimations(newAnimations)
    {
        this->oldAnimations = this->getAnimations();
    }
    ~CommandModifyAnimations() = default;

    void Execute() override {
        this->getAnimations() = this->newAnimations;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getAnimations() = this->oldAnimations;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;

    std::vector<CellAnim::Animation> oldAnimations;
    std::vector<CellAnim::Animation> newAnimations;

    std::vector<CellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations;
    }
};

#endif // COMMANDMODIFYANIMATIONS_HPP
