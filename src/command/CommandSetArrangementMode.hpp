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
        mEnabled(enabled)
    {}
    ~CommandSetArrangementMode() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();
        PlayerManager& playerManager = PlayerManager::getInstance();

        if (mEnabled) {
            playerManager.setPlaying(false);
            playerManager.setArrangementModeIdx(playerManager.getKey().arrangementIndex);
        }

        sessionManager.getCurrentSession()->arrangementMode = mEnabled;

        playerManager.correctState();
    }

    void Rollback() override {
        SessionManager& sessionManager = SessionManager::getInstance();
        PlayerManager& playerManager = PlayerManager::getInstance();

        sessionManager.getCurrentSession()->arrangementMode = !mEnabled;

        playerManager.correctState();
    }

private:
    bool mEnabled;
};

#endif // COMMANDSETARRANGEMENTMODE_HPP
