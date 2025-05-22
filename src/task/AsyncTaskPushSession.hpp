#ifndef ASYNCTASK_PUSHSESSION_HPP
#define ASYNCTASK_PUSHSESSION_HPP

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

#endif // ASYNCTASK_PUSHSESSION_HPP
