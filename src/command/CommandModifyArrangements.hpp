#ifndef COMMANDMODIFYARRANGEMENTS_HPP
#define COMMANDMODIFYARRANGEMENTS_HPP

#include "BaseCommand.hpp"

#include "../cellanim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandModifyArrangements : public BaseCommand {
public:
    // Constructor: Replace arrangements by cellanimIndex by newArrangements.
    CommandModifyArrangements(
        unsigned cellanimIndex,
        std::vector<CellAnim::Arrangement> newArrangements
    ) :
        cellanimIndex(cellanimIndex),
        newArrangements(newArrangements)
    {
        this->oldArrangements = this->getArrangements();
    }
    ~CommandModifyArrangements() = default;

    void Execute() override {
        this->getArrangements() = this->newArrangements;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getArrangements() = this->oldArrangements;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;

    std::vector<CellAnim::Arrangement> oldArrangements;
    std::vector<CellAnim::Arrangement> newArrangements;

    std::vector<CellAnim::Arrangement>& getArrangements() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements;
    }
};

#endif // COMMANDMODIFYARRANGEMENTS_HPP
