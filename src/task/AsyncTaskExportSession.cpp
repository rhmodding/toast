#include "AsyncTaskExportSession.hpp"

#include "manager/SessionManager.hpp"

#include "manager/ConfigManager.hpp"

#include "manager/AppState.hpp"

AsyncTaskExportSession::AsyncTaskExportSession(
    AsyncTaskId id,
    unsigned sessionIndex, std::string filePath
) :
    AsyncTask(id, "Exporting session.."),

    mSessionIndex(sessionIndex), mFilePath(std::move(filePath)),
    mUseSessionPath(false),
    mResult(false)
{}

AsyncTaskExportSession::AsyncTaskExportSession(
    AsyncTaskId id,
    unsigned sessionIndex
) :
    AsyncTask(id, "Exporting session.."),

    mSessionIndex(sessionIndex),
    mUseSessionPath(true)
{}

void AsyncTaskExportSession::run() {
    std::string_view dstFilePath = "";
    if (!mUseSessionPath) {
        dstFilePath = mFilePath;
    }

    bool exportResult = SessionManager::getInstance().exportSession(
        mSessionIndex, dstFilePath
    );
    mResult.store(exportResult);
}

void AsyncTaskExportSession::effect() {
    if (!mResult) {
        return;
    }

    auto& session = SessionManager::getInstance().getSession(mSessionIndex);
    if (!mUseSessionPath) {
        session.resourcePath = mFilePath;
    }

    ConfigManager::getInstance().pushRecentlyOpened(
        mUseSessionPath ? session.resourcePath : mFilePath
    );
}
