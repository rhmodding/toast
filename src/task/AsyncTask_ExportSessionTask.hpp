#ifndef ASYNCTASK_EXPORTSESSIONTASK_HPP
#define ASYNCTASK_EXPORTSESSIONTASK_HPP

#include "AsyncTask.hpp"

#include <string>

#include "../SessionManager.hpp"

#include "../ConfigManager.hpp"

#include "../AppState.hpp"

class ExportSessionTask : public AsyncTask {
public:
    ExportSessionTask(
        uint32_t id,
        SessionManager::Session* session, std::string outPath
    ) :
        AsyncTask(id, "Exporting session.."),

        session(session), outPath(outPath),
        result(0)
    {}

protected:
    void Run() override {
        int tResult = SessionManager::getInstance().ExportSessionCompressedArc(session, outPath.c_str());
        result.store(tResult);
    }

    void Effect() override {
        if (this->result < 0) {
            AppState::getInstance().OpenGlobalPopup("###SessionErr");

            return;
        }

        this->session->traditionalMethod = false;
        this->session->mainPath = outPath;

        GET_CONFIG_MANAGER;

        auto newConfig = configManager.getConfig();

        auto& recentlyOpened = newConfig.recentlyOpened;

        recentlyOpened.erase(std::remove(recentlyOpened.begin(), recentlyOpened.end(), outPath), recentlyOpened.end());
        newConfig.recentlyOpened.push_back(outPath);

        configManager.setConfig(newConfig);
    }

private:
    SessionManager::Session* session;
    std::string outPath;

    std::atomic<int> result;
};

#endif // ASYNCTASK_EXPORTSESSIONTASK_HPP
