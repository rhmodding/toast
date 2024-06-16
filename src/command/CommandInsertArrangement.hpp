#ifndef COMMANDINSERTARRANGEMENT_HPP
#define COMMANDINSERTARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

class CommandInsertArrangement : public BaseCommand {
public:
    // Constructor: Insert arrangement by cellanimIndex and arrangementIndex from arrangement.
    CommandInsertArrangement(
        uint16_t cellanimIndex,
        uint16_t arrangementIndex,
        RvlCellAnim::Arrangement arrangement
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex),
        arrangement(arrangement)
    {}

    void Execute() override {
        auto& arrangements = this->getCellanim()->arrangements;

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.insert(it, this->arrangement);
    }

    void Rollback() override {
        auto& arrangements = this->getCellanim()->arrangements;

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.erase(it);

        for (auto& animation : this->getCellanim()->animations)
            for (auto& key : animation.keys) {
                if (key.arrangementIndex >= arrangements.size())
                    key.arrangementIndex = 0;
            }
    }

private:
    uint16_t cellanimIndex;

    uint16_t arrangementIndex;

    RvlCellAnim::Arrangement arrangement;

    RvlCellAnim::RvlCellAnimObject* getCellanim() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex);
    }
};

#endif // COMMANDINSERTARRANGEMENT_HPP