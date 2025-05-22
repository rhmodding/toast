#include "AsyncTaskExportSession.hpp"

#include "../SessionManager.hpp"

#include "../ConfigManager.hpp"

#include "../AppState.hpp"

AsyncTaskExportSession::AsyncTaskExportSession(
    AsyncTaskId id,
    unsigned sessionIndex, std::string filePath
) :
    AsyncTask(id, "Exporting session.."),

    mSessionIndex(sessionIndex), mFilePath(std::move(filePath)),
    mUseSessionPath(false)
{}

AsyncTaskExportSession::AsyncTaskExportSession(
    AsyncTaskId id,
    unsigned sessionIndex
) :
    AsyncTask(id, "Exporting session.."),

    mSessionIndex(sessionIndex),
    mUseSessionPath(true)
{}

void AsyncTaskExportSession::Run() {
    std::string_view dstFilePath = "";
    if (!mUseSessionPath) {
        dstFilePath = mFilePath;
    }

    bool exportResult = SessionManager::getInstance().ExportSession(
        mSessionIndex, dstFilePath
    );
    mResult.store(exportResult);
}

void AsyncTaskExportSession::Effect() {
    if (!mResult) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    auto& session = SessionManager::getInstance().getSession(mSessionIndex);
    if (!mUseSessionPath) {
        session.resourcePath = mFilePath;
    }

    ConfigManager::getInstance().addRecentlyOpened(
        mUseSessionPath ? session.resourcePath : mFilePath
    );
}
