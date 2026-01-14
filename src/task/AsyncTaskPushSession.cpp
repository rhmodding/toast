#include "AsyncTaskPushSession.hpp"

#include "manager/SessionManager.hpp"
#include "manager/ConfigManager.hpp"

#include "manager/AppState.hpp"

AsyncTaskPushSession::AsyncTaskPushSession(uint32_t id, std::string filePath) :
    AsyncTask(id, "Opening session.."),

    mFilePath(std::move(filePath)),
    mResult(0)
{}

void AsyncTaskPushSession::run() {
    ssize_t pushResult = SessionManager::getInstance().createSession(mFilePath);
    mResult.store(pushResult);
}

void AsyncTaskPushSession::effect() {
    if (mResult < 0) {
        return;
    }

    SessionManager::getInstance().setCurrentSessionIndex(mResult);

    ConfigManager::getInstance().pushRecentlyOpened(mFilePath);
}