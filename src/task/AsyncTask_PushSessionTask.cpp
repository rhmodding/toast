#include "AsyncTask_PushSessionTask.hpp"

#include "../SessionManager.hpp"
#include "../ConfigManager.hpp"

#include "../AppState.hpp"

PushSessionTask::PushSessionTask(uint32_t id, std::string filePath) :
    AsyncTask(id, "Opening session.."), filePath(filePath)
{}

void PushSessionTask::Run() {
    int pushResult = SessionManager::getInstance().PushSessionFromCompressedArc(this->filePath.c_str());
    this->result.store(pushResult);
}

void PushSessionTask::Effect() {
    GET_SESSION_MANAGER;

    if (this->result < 0) {
        AppState::getInstance().OpenGlobalPopup("###SessionErr");
        return;
    }

    sessionManager.currentSessionIndex = this->result;
    sessionManager.SessionChanged();

    GET_CONFIG_MANAGER;

    auto newConfig = configManager.getConfig();

    auto& recentlyOpened = newConfig.recentlyOpened;

    recentlyOpened.erase(std::remove(recentlyOpened.begin(), recentlyOpened.end(), filePath), recentlyOpened.end());
    newConfig.recentlyOpened.push_back(filePath);

    configManager.setConfig(newConfig);
}