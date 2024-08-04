#ifndef COMMANDMODIFYARRANGEMENT_HPP
#define COMMANDMODIFYARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandModifyArrangement : public BaseCommand {
public:
    // Constructor: Replace arrangement from cellanimIndex and arrangementIndex by newArrangement.
    CommandModifyArrangement(
        uint16_t cellanimIndex, uint32_t arrangementIndex,
        RvlCellAnim::Arrangement newArrangement
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex),
        newArrangement(newArrangement)
    {
        this->oldArrangement = this->getArrangement();
    }

    void Execute() override {
        this->getArrangement() = this->newArrangement;

        AppState::getInstance().correctSelectedPart();
    }

    void Rollback() override {
        this->getArrangement() = this->oldArrangement;

        AppState::getInstance().correctSelectedPart();
    }

private:
    uint16_t cellanimIndex;
    uint32_t arrangementIndex;

    RvlCellAnim::Arrangement oldArrangement;
    RvlCellAnim::Arrangement newArrangement;

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENT_HPP
