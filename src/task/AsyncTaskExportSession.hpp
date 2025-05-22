#ifndef ASYNCTASK_EXPORTSESSION_HPP
#define ASYNCTASK_EXPORTSESSION_HPP

#include "AsyncTask.hpp"

#include <string>

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
    unsigned mSessionIndex;
    std::string mFilePath;

    bool mUseSessionPath;

    std::atomic<bool> mResult { 0 };
};

#endif // ASYNCTASK_EXPORTSESSION_HPP
