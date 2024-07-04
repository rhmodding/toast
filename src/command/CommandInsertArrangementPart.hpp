#ifndef COMMANDINSERTARRANGEMENTPART_HPP
#define COMMANDINSERTARRANGEMENTPART_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

#include <algorithm>

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
    }

    void Rollback() override {
        RvlCellAnim::Arrangement& arrangement = this->getArrangement();

        auto it = arrangement.parts.begin() + this->partIndex;
        arrangement.parts.erase(it);

        GET_APP_STATE;
        appState.selectedPart = std::clamp<int32_t>(
            appState.selectedPart,
            -1,
            static_cast<int32_t>(arrangement.parts.size() - 1)
        );
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