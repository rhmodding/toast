#ifndef COMMANDDELETEARRANGEMENT_HPP
#define COMMANDDELETEARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include <algorithm>

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandDeleteArrangement : public BaseCommand {
public:
    // Constructor: Delete arrangement by cellanimIndex and arrangementIndex.
    CommandDeleteArrangement(
        unsigned cellanimIndex, unsigned arrangementIndex
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex)
    {
        this->arrangement = this->getArrangement();
    }
    ~CommandDeleteArrangement() = default;

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
            PlayerManager::getInstance().clampCurrentKeyIndex();
        else
            appState.controlKey.arrangementIndex = std::min<uint16_t>(
                appState.controlKey.arrangementIndex,
                arrangements.size() - 1
            );

        appState.correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
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
            PlayerManager::getInstance().clampCurrentKeyIndex();
        else
            appState.controlKey.arrangementIndex = std::min<uint16_t>(
                appState.controlKey.arrangementIndex,
                arrangements.size() - 1
            );

        appState.correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;

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
