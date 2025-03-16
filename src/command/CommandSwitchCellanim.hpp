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
        this->previousCellanim =
            SessionManager::getInstance().sessions.at(this->sessionIndex).getCurrentCellanimIndex();
    }
    ~CommandSwitchCellanim() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        auto& session = sessionManager.sessions.at(this->sessionIndex);
        session.setCurrentCellanimIndex(this->cellanimIndex);

        PlayerManager::getInstance().correctState();
    }

    void Rollback() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        auto& session = sessionManager.sessions.at(this->sessionIndex);
        session.setCurrentCellanimIndex(this->previousCellanim);

        PlayerManager::getInstance().correctState();
    }

private:
    unsigned sessionIndex;

    unsigned cellanimIndex;
    unsigned previousCellanim;
};

#endif // COMMANDSWITCHCELLANIM_HPP
