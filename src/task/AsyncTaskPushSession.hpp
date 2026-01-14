#ifndef ASYNC_TASK_PUSHSESSION_HPP
#define ASYNC_TASK_PUSHSESSION_HPP

#include "AsyncTask.hpp"

#include <string>

class AsyncTaskPushSession : public AsyncTask {
public:
    AsyncTaskPushSession(uint32_t id, std::string filePath);

protected:
    void run() override;
    void effect() override;

private:
    std::string mFilePath;

    std::atomic<ssize_t> mResult;
};

#endif // ASYNC_TASK_PUSHSESSION_HPP
