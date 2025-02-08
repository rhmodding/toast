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
    ~CommandSetArrangementMode() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        sessionManager.getCurrentSession()->arrangementMode = this->enabled;

        this->UpdateState(this->enabled);
    }

    void Rollback() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        sessionManager.getCurrentSession()->arrangementMode = !this->enabled;

        this->UpdateState(!this->enabled);
    }

private:
    void UpdateState(bool lEnabled) {
        AppState& appState = AppState::getInstance();

        appState.correctSelectedParts();

        auto& globalAnimatable = appState.globalAnimatable;

        if (lEnabled) {
            appState.controlKey.arrangementIndex = globalAnimatable.getCurrentKey()->arrangementIndex;

            PlayerManager::getInstance().setPlaying(false);
            globalAnimatable.overrideAnimationKey(&appState.controlKey);
        }
        else {
            globalAnimatable.setAnimationFromIndex(appState.selectedAnimation);
        }
    }

    bool enabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP
