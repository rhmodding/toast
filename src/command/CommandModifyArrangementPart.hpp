#ifndef COMMANDMODIFYARRANGEMENTPART_HPP
#define COMMANDMODIFYARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "../cellanim/CellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandModifyArrangementPart : public BaseCommand {
public:
    // Constructor: Replace part from cellanimIndex, arrangementIndex and partIndex by newPart.
    CommandModifyArrangementPart(
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex,
        CellAnim::ArrangementPart newPart
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex),
        newPart(newPart)
    {
        this->oldPart = this->getPart();
    }
    ~CommandModifyArrangementPart() = default;

    void Execute() override {
        this->getPart() = this->newPart;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getPart() = this->oldPart;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;
    unsigned partIndex;

    CellAnim::ArrangementPart oldPart;
    CellAnim::ArrangementPart newPart;

    CellAnim::ArrangementPart& getPart() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex)
            .parts.at(this->partIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENTPART_HPP
