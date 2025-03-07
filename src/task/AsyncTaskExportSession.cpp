#include "AsyncTaskExportSession.hpp"

AsyncTaskExportSession::AsyncTaskExportSession(
    AsyncTaskId id,
    unsigned sessionIndex, std::string filePath
) :
    AsyncTask(id, "Exporting session.."),

    sessionIndex(sessionIndex), filePath(std::move(filePath)),
    useSessionPath(false)
{}

AsyncTaskExportSession::AsyncTaskExportSession(
    AsyncTaskId id,
    unsigned sessionIndex
) :
    AsyncTask(id, "Exporting session.."),

    sessionIndex(sessionIndex),
    useSessionPath(true)
{}

void AsyncTaskExportSession::Run() {
    int exportResult = SessionManager::getInstance().ExportSession(
        this->sessionIndex,
        this->useSessionPath ? nullptr : this->filePath.c_str()
    );
    this->result.store(exportResult);
}

void AsyncTaskExportSession::Effect() {
    if (this->result < 0) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    auto& session = SessionManager::getInstance().sessions.at(this->sessionIndex);
    if (!this->useSessionPath)
        session.resourcePath = this->filePath;

    ConfigManager::getInstance().addRecentlyOpened(this->filePath);
}
