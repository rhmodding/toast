#ifndef COMMANDSETARRANGEMENTMODE_HPP
#define COMMANDSETARRANGEMENTMODE_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

class CommandSetArrangementMode : public BaseCommand {
public:
    // Constructor: Set arrangement mode in a session's scope.
    CommandSetArrangementMode(
        uint32_t sessionIndex,
        bool enabled
    ) :
        sessionIndex(sessionIndex),
        enabled(enabled)
    {}

    void Execute() override {
        GET_SESSION_MANAGER;

        sessionManager.sessionList.at(this->sessionIndex).arrangementMode = enabled;
    }

    void Rollback() override {
        GET_SESSION_MANAGER;

        sessionManager.sessionList.at(this->sessionIndex).arrangementMode = !enabled;
    }

private:
    uint32_t sessionIndex;
    
    bool enabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP