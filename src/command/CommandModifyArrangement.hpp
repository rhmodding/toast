#ifndef COMMANDMODIFYARRANGEMENT_HPP
#define COMMANDMODIFYARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandModifyArrangement : public BaseCommand {
public:
    // Constructor: Replace arrangement from cellanimIndex and arrangementIndex by newArrangement.
    CommandModifyArrangement(
        unsigned cellanimIndex, unsigned arrangementIndex,
        CellAnim::Arrangement newArrangement
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex),
        newArrangement(newArrangement)
    {
        this->oldArrangement = this->getArrangement();
    }
    ~CommandModifyArrangement() = default;

    void Execute() override {
        this->getArrangement() = this->newArrangement;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getArrangement() = this->oldArrangement;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;

    CellAnim::Arrangement oldArrangement;
    CellAnim::Arrangement newArrangement;

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDMODIFYARRANGEMENT_HPP
