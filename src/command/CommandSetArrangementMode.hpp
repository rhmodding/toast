#ifndef COMMANDSETARRANGEMENTMODE_HPP
#define COMMANDSETARRANGEMENTMODE_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

class CommandSetArrangementMode : public BaseCommand {
public:
    // Constructor: Set arrangement mode in the current session's scope.
    CommandSetArrangementMode(
        bool enabled
    ) :
        enabled(enabled)
    {}

    void Execute() override {
        GET_SESSION_MANAGER;

        sessionManager.getCurrentSession()->arrangementMode = this->enabled;

        this->UpdateState(this->enabled);
    }

    void Rollback() override {
        GET_SESSION_MANAGER;

        sessionManager.getCurrentSession()->arrangementMode = !this->enabled;

        this->UpdateState(!this->enabled);
    }

private:
    void UpdateState(bool lEnabled) {
        GET_APP_STATE;

        appState.correctSelectedParts();

        auto& globalAnimatable = appState.globalAnimatable;

        if (lEnabled) {
            appState.controlKey.arrangementIndex = globalAnimatable->getCurrentKey()->arrangementIndex;

            PlayerManager::getInstance().setAnimating(false);
            globalAnimatable->overrideAnimationKey(&appState.controlKey);
        }
        else {
            globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);
        }
    }

    bool enabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP
