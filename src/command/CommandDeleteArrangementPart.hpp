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
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex)
    {
        this->part = this->getPart();
    }

    void Execute() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.erase(it);

        AppState::getInstance().correctSelectedParts();
    }

    void Rollback() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.insert(it, this->part);
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;
    unsigned partIndex;

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
