#ifndef COMMANDINSERTARRANGEMENTPART_HPP
#define COMMANDINSERTARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandInsertArrangementPart : public BaseCommand {
public:
    // Constructor: Insert part by cellanimIndex, arrangementIndex and partIndex from part.
    CommandInsertArrangementPart(
        uint16_t cellanimIndex, uint32_t arrangementIndex, uint32_t partIndex,
        RvlCellAnim::ArrangementPart part
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex),
        part(part)
    {}

    void Execute() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.insert(it, this->part);

        AppState::getInstance().globalAnimatable->refreshPointers();
    }

    void Rollback() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.erase(it);

        AppState::getInstance().globalAnimatable->refreshPointers();

        AppState::getInstance().correctSelectedPart();
    }

private:
    uint16_t cellanimIndex;
    uint32_t arrangementIndex;
    uint32_t partIndex;

    RvlCellAnim::ArrangementPart part;

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDINSERTARRANGEMENTPART_HPP
