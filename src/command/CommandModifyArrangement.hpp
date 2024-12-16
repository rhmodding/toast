#ifndef COMMANDMODIFYARRANGEMENT_HPP
#define COMMANDMODIFYARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandModifyArrangement : public BaseCommand {
public:
    // Constructor: Replace arrangement from cellanimIndex and arrangementIndex by newArrangement.
    CommandModifyArrangement(
        unsigned cellanimIndex, unsigned arrangementIndex,
        RvlCellAnim::Arrangement newArrangement
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex),
        newArrangement(newArrangement)
    {
        this->oldArrangement = this->getArrangement();
    }
    ~CommandModifyArrangement() = default;

    void Execute() override {
        this->getArrangement() = this->newArrangement;

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        this->getArrangement() = this->oldArrangement;

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;

    RvlCellAnim::Arrangement oldArrangement;
    RvlCellAnim::Arrangement newArrangement;

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENT_HPP
