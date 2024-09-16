#ifndef ASYNCTASK_PUSHSESSIONTASK_HPP
#define ASYNCTASK_PUSHSESSIONTASK_HPP

#include "AsyncTask.hpp"

#include <string>

#include "../SessionManager.hpp"

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
        int tResult = SessionManager::getInstance().PushSessionFromCompressedArc(filePath.c_str());
        result.store(tResult);
    }

    void Effect() override {
        GET_SESSION_MANAGER;

        if (UNLIKELY(this->result < 0)) {
            AppState::getInstance().OpenGlobalPopup("###SessionErr");

            return;
        }

        sessionManager.currentSession = result;
        sessionManager.SessionChanged();
    }

private:
    SessionManager::Session* session;
    std::string filePath;

    std::atomic<int> result;
};

#endif // ASYNCTASK_PUSHSESSIONTASK_HPP
