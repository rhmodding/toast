#ifndef COMMANDSWITCHCELLANIM_HPP
#define COMMANDSWITCHCELLANIM_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

#include "../PlayerManager.hpp"

class CommandSwitchCellanim : public BaseCommand {
public:
    // Constructor: Switch cellanim space by index.
    CommandSwitchCellanim(
        unsigned sessionIndex,
        unsigned cellanimIndex
    ) :
        sessionIndex(sessionIndex),
        cellanimIndex(cellanimIndex)
    {
        this->previousCellAnim =
            SessionManager::getInstance().sessions.at(this->sessionIndex).getCurrentCellAnimIndex();
    }
    ~CommandSwitchCellanim() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        auto& session = sessionManager.sessions.at(this->sessionIndex);
        session.setCurrentCellAnimIndex(this->cellanimIndex);

        PlayerManager::getInstance().correctState();
    }

    void Rollback() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        auto& session = sessionManager.sessions.at(this->sessionIndex);
        session.setCurrentCellAnimIndex(this->previousCellAnim);

        PlayerManager::getInstance().correctState();
    }

private:
    unsigned sessionIndex;

    unsigned cellanimIndex;
    unsigned previousCellAnim;
};

#endif // COMMANDSWITCHCELLANIM_HPP
