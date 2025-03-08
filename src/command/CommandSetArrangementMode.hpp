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
        (void)appState;
        PlayerManager& playerManager = PlayerManager::getInstance();

        playerManager.correctState();

        if (lEnabled) {
            // TODO
            // appState.controlKey.arrangementIndex = playerManager.getKey().arrangementIndex;

            playerManager.setPlaying(false);
            // TODO
            // globalAnimatable.overrideAnimationKey(&appState.controlKey);
        }
    }

    bool enabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP
