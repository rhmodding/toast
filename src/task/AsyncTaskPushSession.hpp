#ifndef ASYNC_TASK_PUSHSESSION_HPP
#define ASYNC_TASK_PUSHSESSION_HPP

#include "AsyncTask.hpp"

#include <string>

class AsyncTaskPushSession : public AsyncTask {
public:
    AsyncTaskPushSession(uint32_t id, std::string filePath);

protected:
    void Run() override;
    void Effect() override;

private:
    std::string mFilePath;

    std::atomic<int> mResult { 0 };
};

#endif // ASYNC_TASK_PUSHSESSION_HPP
