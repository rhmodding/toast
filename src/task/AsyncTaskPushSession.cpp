#include "AsyncTaskPushSession.hpp"

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"

#include "../AppState.hpp"

AsyncTaskPushSession::AsyncTaskPushSession(uint32_t id, std::string filePath) :
    AsyncTask(id, "Opening session.."),

    filePath(std::move(filePath))
{}

void AsyncTaskPushSession::Run() {
    int pushResult = SessionManager::getInstance().CreateSession(this->filePath.c_str());
    this->result.store(pushResult);
}

void AsyncTaskPushSession::Effect() {
    if (this->result < 0) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    SessionManager::getInstance().setCurrentSessionIndex(this->result);

    ConfigManager::getInstance().addRecentlyOpened(filePath);
}