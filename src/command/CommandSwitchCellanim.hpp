#ifndef COMMANDSWITCHCELLANIM_HPP
#define COMMANDSWITCHCELLANIM_HPP

#include "BaseCommand.hpp"

#include "manager/SessionManager.hpp"

#include "manager/PlayerManager.hpp"

class CommandSwitchCellanim : public BaseCommand {
public:
    // Constructor: Switch cellanim space by index.
    CommandSwitchCellanim(
        unsigned sessionIndex,
        unsigned cellanimIndex
    ) :
        mSessionIndex(sessionIndex),
        mCellAnimIndex(cellanimIndex)
    {
        mPreviousCellAnimIndex =
            SessionManager::getInstance().getSession(mSessionIndex).getCurrentCellAnimIndex();
    }
    ~CommandSwitchCellanim() = default;

    void Execute() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        auto& session = sessionManager.getSession(mSessionIndex);
        session.setCurrentCellAnimIndex(mCellAnimIndex);

        PlayerManager::getInstance().correctState();
    }

    void Rollback() override {
        SessionManager& sessionManager = SessionManager::getInstance();

        auto& session = sessionManager.getSession(mSessionIndex);
        session.setCurrentCellAnimIndex(mPreviousCellAnimIndex);

        PlayerManager::getInstance().correctState();
    }

private:
    unsigned mSessionIndex;

    unsigned mCellAnimIndex;
    unsigned mPreviousCellAnimIndex;
};

#endif // COMMANDSWITCHCELLANIM_HPP
