#ifndef COMMANDMODIFYARRANGEMENTPART_HPP
#define COMMANDMODIFYARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandModifyArrangementPart : public BaseCommand {
public:
    // Constructor: Replace part from cellanimIndex, arrangementIndex and partIndex by newPart.
    CommandModifyArrangementPart(
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex,
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
        this->partIndex = AppState::getInstance().selectedParts.at(0).index;

        this->oldPart = this->getPart();
    }

    void Execute() override {
        this->getPart() = this->newPart;
    }

    void Rollback() override {
        this->getPart() = this->oldPart;
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;
    unsigned partIndex;

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
