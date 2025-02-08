#include "AsyncTask_ExportSessionTask.hpp"

ExportSessionTask::ExportSessionTask(
    uint32_t id,
    Session* session, std::string outPath
) :
    AsyncTask(id, "Exporting session.."),

    session(session), outPath(outPath)
{}

void ExportSessionTask::Run() {
    int exportResult = SessionManager::getInstance().ExportSessionCompressedArc(
        this->session, this->outPath.c_str()
    );
    this->result.store(exportResult);
}

void ExportSessionTask::Effect() {
    if (this->result < 0) {
        OPEN_GLOBAL_POPUP("###SessionErr");
        return;
    }

    this->session->traditionalMethod = false;
    this->session->mainPath = this->outPath;

    ConfigManager::getInstance().addRecentlyOpened(this->outPath);
}
