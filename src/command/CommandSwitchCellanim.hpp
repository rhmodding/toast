#ifndef COMMANDSWITCHCELLANIM_HPP
#define COMMANDSWITCHCELLANIM_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

class CommandSwitchCellanim : public BaseCommand {
public:
    // Constructor: Switch cellanim space by index.
    CommandSwitchCellanim(
        uint32_t sessionIndex,
        uint16_t cellanimIndex
    ) :
        sessionIndex(sessionIndex),
        cellanimIndex(cellanimIndex)
    {
        this->previousCellanim = SessionManager::getInstance().sessionList.at(this->sessionIndex).currentCellanim;
    }

    void Execute() override {
        GET_SESSION_MANAGER;

        sessionManager.sessionList.at(this->sessionIndex).currentCellanim = this->cellanimIndex;
        sessionManager.SessionChanged();
    }

    void Rollback() override {
        GET_SESSION_MANAGER;

        sessionManager.sessionList.at(this->sessionIndex).currentCellanim = this->previousCellanim;
        sessionManager.SessionChanged();
    }

private:
    uint16_t cellanimIndex;
    uint16_t previousCellanim;

    uint32_t sessionIndex;
};

#endif // COMMANDSWITCHCELLANIM_HPP