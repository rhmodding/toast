#ifndef COMMANDDELETEARRANGEMENT_HPP
#define COMMANDDELETEARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

#include <algorithm>

class CommandDeleteArrangement : public BaseCommand {
public:
    // Constructor: Delete arrangement by cellanimIndex and arrangementIndex.
    CommandDeleteArrangement(
        uint16_t cellanimIndex, uint32_t arrangementIndex
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex)
    {
        this->arrangement = this->getArrangement();
    }

    void Execute() override {
        auto& arrangements = this->getArrangements();

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.erase(it);

        for (
            auto& animation :
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object->animations
        ) for (auto& key : animation.keys) {
            if (key.arrangementIndex > arrangementIndex)
                key.arrangementIndex--;
            else if (key.arrangementIndex == arrangementIndex)
                key.arrangementIndex = 0;
        }

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            appState.playerState.updateCurrentFrame();
        else
            appState.controlKey.arrangementIndex = std::clamp<uint16_t>(
                appState.controlKey.arrangementIndex,
                0,
                arrangements.size() - 1
            );

        appState.correctSelectedPart();
    }

    void Rollback() override {
        auto& arrangements = this->getArrangements();

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.insert(it, this->arrangement);

        for (
            auto& animation :
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object->animations
        ) for (auto& key : animation.keys) {
            if (key.arrangementIndex >= arrangementIndex)
                key.arrangementIndex++;
        }

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            appState.playerState.updateCurrentFrame();
        else
            appState.controlKey.arrangementIndex = std::clamp<uint16_t>(
                appState.controlKey.arrangementIndex,
                0,
                arrangements.size() - 1
            );

        appState.correctSelectedPart();
    }

private:
    uint16_t cellanimIndex;
    uint32_t arrangementIndex;

    RvlCellAnim::Arrangement arrangement;

    std::vector<RvlCellAnim::Arrangement>& getArrangements() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements;
    }

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDDELETEARRANGEMENT_HPP