#ifndef COMMANDSETARRANGEMENTMODE_HPP
#define COMMANDSETARRANGEMENTMODE_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

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

        appState.correctSelectedPart();

        auto& globalAnimatable = appState.globalAnimatable;

        if (lEnabled) {
            appState.controlKey.arrangementIndex = globalAnimatable->getCurrentKey()->arrangementIndex;

            appState.playerState.ToggleAnimating(false);
            globalAnimatable->overrideAnimationKey(&appState.controlKey);
        }
        else {
            globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);
        }
    }

    bool enabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP