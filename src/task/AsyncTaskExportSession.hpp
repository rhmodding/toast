#ifndef ASYNC_TASK_EXPORTSESSION_HPP
#define ASYNC_TASK_EXPORTSESSION_HPP

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
    void run() override;
    void effect() override;

private:
    unsigned mSessionIndex;
    std::string mFilePath;

    bool mUseSessionPath;

    std::atomic<bool> mResult;
};

#endif // ASYNC_TASK_EXPORTSESSION_HPP
