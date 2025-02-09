#include "AsyncTaskPushSession.hpp"

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"

#include "../AppState.hpp"

AsyncTaskPushSession::AsyncTaskPushSession(uint32_t id, std::string filePath) :
    AsyncTask(id, "Opening session.."), filePath(filePath)
{}

void AsyncTaskPushSession::Run() {
    int pushResult = SessionManager::getInstance().PushSessionFromCompressedArc(this->filePath.c_str());
    this->result.store(pushResult);
}

void AsyncTaskPushSession::Effect() {
    SessionManager& sessionManager = SessionManager::getInstance();

    if (this->result < 0) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    sessionManager.currentSessionIndex = this->result;
    sessionManager.SessionChanged();

    ConfigManager::getInstance().addRecentlyOpened(filePath);
}