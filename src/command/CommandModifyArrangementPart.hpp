#ifndef COMMANDMODIFYARRANGEMENTPART_HPP
#define COMMANDMODIFYARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

class CommandModifyArrangementPart : public BaseCommand {
public:
    // Constructor: Replace part from cellanimIndex, arrangementIndex and partIndex by newPart.
    CommandModifyArrangementPart(
        uint16_t cellanimIndex, uint32_t arrangementIndex, uint32_t partIndex,
        RvlCellAnim::ArrangementPart newPart
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex),
        newPart(newPart)
    {
        this->oldPart = this->getPart();
    }

    // Constructor: Modify selected part.
    CommandModifyArrangementPart(
        RvlCellAnim::ArrangementPart newPart
    ) :
        newPart(newPart)
    {
        this->cellanimIndex = SessionManager::getInstance().getCurrentSession()->currentCellanim;
        this->arrangementIndex = AppState::getInstance().globalAnimatable->getCurrentKey()->arrangementIndex;
        this->partIndex = AppState::getInstance().selectedPart;

        this->oldPart = this->getPart();
    }

    void Execute() override {
        this->getPart() = this->newPart;
    }

    void Rollback() override {
        this->getPart() = this->oldPart;
    }

private:
    uint16_t cellanimIndex;
    uint32_t arrangementIndex;
    uint32_t partIndex;

    RvlCellAnim::ArrangementPart oldPart;
    RvlCellAnim::ArrangementPart newPart;

    RvlCellAnim::ArrangementPart& getPart() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex)
            .parts.at(this->partIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENTPART_HPP