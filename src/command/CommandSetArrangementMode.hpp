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

        appState.selectedPart = std::clamp<int32_t>(
            appState.selectedPart,
            -1,
            appState.globalAnimatable->getCurrentArrangement()->parts.size() - 1
        );

        auto& globalAnimatable = appState.globalAnimatable;

        if (lEnabled) {
            appState.controlKey.arrangementIndex = globalAnimatable->getCurrentKey()->arrangementIndex;

            appState.playerState.ToggleAnimating(false);
            globalAnimatable->setAnimationKeyFromPtr(&appState.controlKey);
        }
        else {
            globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);
        }
    }

    bool enabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP