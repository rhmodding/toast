#ifndef COMMANDDELETEARRANGEMENTPART_HPP
#define COMMANDDELETEARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

class CommandDeleteArrangementPart : public BaseCommand {
public:
    // Constructor: Delete part by cellanimIndex, arrangementIndex and partIndex.
    CommandDeleteArrangementPart(
        uint16_t cellanimIndex, uint32_t arrangementIndex, uint32_t partIndex
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex)
    {
        this->part = this->getPart();
    }

    void Execute() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.erase(it);

        AppState::getInstance().correctSelectedPart();
    }

    void Rollback() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.insert(it, this->part);
    }

private:
    uint16_t cellanimIndex;
    uint32_t arrangementIndex;
    uint32_t partIndex;

    RvlCellAnim::ArrangementPart part;

    RvlCellAnim::ArrangementPart& getPart() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex)
            .parts.at(this->partIndex);
    }

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDDELETEARRANGEMENTPART_HPP
