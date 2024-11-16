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
        unsigned cellanimIndex, unsigned arrangementIndex, unsigned partIndex,
        RvlCellAnim::ArrangementPart part
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex), partIndex(partIndex),
        part(part)
    {}

    void Execute() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.insert(it, this->part);

        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.erase(it);

        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;
    unsigned partIndex;

    RvlCellAnim::ArrangementPart part;

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDINSERTARRANGEMENTPART_HPP
