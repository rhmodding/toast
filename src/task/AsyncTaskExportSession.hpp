#ifndef ASYNCTASK_EXPORTSESSION_HPP
#define ASYNCTASK_EXPORTSESSION_HPP

#include "AsyncTask.hpp"

#include <string>

#include "../SessionManager.hpp"

#include "../ConfigManager.hpp"

#include "../AppState.hpp"

class AsyncTaskExportSession : public AsyncTask {
public:
    AsyncTaskExportSession(
        AsyncTaskId id,
        unsigned sessionIndex, std::string filePath
    );
    AsyncTaskExportSession(
        AsyncTaskId id,
        unsigned sessionIndex
    );

protected:
    void Run() override;
    void Effect() override;

private:
    unsigned sessionIndex;

    bool useSessionPath;
    std::string filePath;

    std::atomic<int> result { 0 };
};

#endif // ASYNCTASK_EXPORTSESSION_HPP
