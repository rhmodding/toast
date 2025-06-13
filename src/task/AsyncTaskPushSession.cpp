#include "AsyncTaskPushSession.hpp"

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"

#include "manager/AppState.hpp"

AsyncTaskPushSession::AsyncTaskPushSession(uint32_t id, std::string filePath) :
    AsyncTask(id, "Opening session.."),

    mFilePath(std::move(filePath))
{}

void AsyncTaskPushSession::Run() {
    int pushResult = SessionManager::getInstance().CreateSession(mFilePath.c_str());
    mResult.store(pushResult);
}

void AsyncTaskPushSession::Effect() {
    if (mResult < 0) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    SessionManager::getInstance().setCurrentSessionIndex(mResult);

    ConfigManager::getInstance().addRecentlyOpened(mFilePath);
}