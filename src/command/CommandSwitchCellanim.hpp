#ifndef COMMANDSWITCHCELLANIM_HPP
#define COMMANDSWITCHCELLANIM_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

class CommandSwitchCellanim : public BaseCommand {
public:
    // Constructor: Switch cellanim space by index.
    CommandSwitchCellanim(
        unsigned sessionIndex,
        unsigned cellanimIndex
    ) :
        cellanimIndex(cellanimIndex),
        sessionIndex(sessionIndex)
    {
        this->previousCellanim = SessionManager::getInstance().sessionList.at(this->sessionIndex).currentCellanim;
    }
    ~CommandSwitchCellanim() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        sessionManager.sessionList.at(this->sessionIndex).currentCellanim = this->cellanimIndex;
        sessionManager.SessionChanged();
    }

    void Rollback() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        sessionManager.sessionList.at(this->sessionIndex).currentCellanim = this->previousCellanim;
        sessionManager.SessionChanged();
    }

private:
    unsigned cellanimIndex;
    unsigned previousCellanim;

    unsigned sessionIndex;
};

#endif // COMMANDSWITCHCELLANIM_HPP