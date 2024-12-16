#ifndef ASYNCTASK_PUSHSESSIONTASK_HPP
#define ASYNCTASK_PUSHSESSIONTASK_HPP

#include "AsyncTask.hpp"

#include <string>

#include "../SessionManager.hpp"

#include "../ConfigManager.hpp"

#include "../AppState.hpp"

class PushSessionTask : public AsyncTask {
public:
    PushSessionTask(
        uint32_t id,
        std::string filePath
    ) :
        AsyncTask(id, "Opening session.."),

        filePath(filePath),
        result(0)
    {}

protected:
    void Run() override {
        int lResult = SessionManager::getInstance().PushSessionFromCompressedArc(filePath.c_str());
        result.store(lResult);
    }

    void Effect() override {
        GET_SESSION_MANAGER;

        if (this->result < 0) {
            AppState::getInstance().OpenGlobalPopup("###SessionErr");

            return;
        }

        sessionManager.currentSessionIndex = result;
        sessionManager.SessionChanged();

        GET_CONFIG_MANAGER;

        auto newConfig = configManager.getConfig();

        auto& recentlyOpened = newConfig.recentlyOpened;

        recentlyOpened.erase(std::remove(recentlyOpened.begin(), recentlyOpened.end(), filePath), recentlyOpened.end());
        newConfig.recentlyOpened.push_back(filePath);

        configManager.setConfig(newConfig);
    }

private:
    std::string filePath;

    std::atomic<int> result;
};

#endif // ASYNCTASK_PUSHSESSIONTASK_HPP
