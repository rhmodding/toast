#include "AsyncTaskExportSession.hpp"

AsyncTaskExportSession::AsyncTaskExportSession(
    uint32_t id,
    Session* session, std::string outPath
) :
    AsyncTask(id, "Exporting session.."),

    session(session), outPath(outPath)
{}

void AsyncTaskExportSession::Run() {
    int exportResult = SessionManager::getInstance().ExportSessionCompressedArc(
        this->session, this->outPath.c_str()
    );
    this->result.store(exportResult);
}

void AsyncTaskExportSession::Effect() {
    if (this->result < 0) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    this->session->traditionalMethod = false;
    this->session->mainPath = this->outPath;

    ConfigManager::getInstance().addRecentlyOpened(this->outPath);
}
