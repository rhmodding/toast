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
        Session* session, std::string outPath
    );

protected:
    void Run() override;
    void Effect() override;

private:
    Session* session;
    std::string outPath;

    std::atomic<int> result { 0 };
};

#endif // ASYNCTASK_EXPORTSESSIONTASK_HPP
