#ifndef COMMANDINSERTARRANGEMENT_HPP
#define COMMANDINSERTARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../AppState.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

class CommandInsertArrangement : public BaseCommand {
public:
    // Constructor: Insert arrangement by cellanimIndex and arrangementIndex from arrangement.
    CommandInsertArrangement(
        unsigned cellanimIndex,
        unsigned arrangementIndex,
        RvlCellAnim::Arrangement arrangement
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex),
        arrangement(arrangement)
    {}

    void Execute() override {
        auto& arrangements = this->getCellanim()->arrangements;

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.insert(it, this->arrangement);

        AppState::getInstance().globalAnimatable->refreshPointers();
    }

    void Rollback() override {
        auto& arrangements = this->getCellanim()->arrangements;

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.erase(it);

        AppState::getInstance().globalAnimatable->refreshPointers();

        for (auto& animation : this->getCellanim()->animations)
            for (auto& key : animation.keys) {
                if (key.arrangementIndex >= arrangements.size())
                    key.arrangementIndex = 0;
            }
    }

private:
    unsigned cellanimIndex;

    unsigned arrangementIndex;

    RvlCellAnim::Arrangement arrangement;

    std::shared_ptr<RvlCellAnim::RvlCellAnimObject> getCellanim() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object;
    }
};

#endif // COMMANDINSERTARRANGEMENT_HPP
