#include "AsyncTask_ExportSessionTask.hpp"

ExportSessionTask::ExportSessionTask(
    uint32_t id,
    SessionManager::Session* session, std::string outPath
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
        AppState::getInstance().OpenGlobalPopup("###SessionErr");
        return;
    }

    this->session->traditionalMethod = false;
    this->session->mainPath = this->outPath;

    GET_CONFIG_MANAGER;

    auto newConfig = configManager.getConfig();

    auto& recentlyOpened = newConfig.recentlyOpened;

    recentlyOpened.erase(std::remove(recentlyOpened.begin(), recentlyOpened.end(), this->outPath), recentlyOpened.end());
    newConfig.recentlyOpened.push_back(this->outPath);

    configManager.setConfig(newConfig);
}


