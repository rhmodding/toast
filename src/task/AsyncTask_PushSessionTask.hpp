#ifndef ASYNCTASK_PUSHSESSIONTASK_HPP
#define ASYNCTASK_PUSHSESSIONTASK_HPP

#include "AsyncTask.hpp"

#include <string>

class PushSessionTask : public AsyncTask {
public:
    PushSessionTask(uint32_t id, std::string filePath);

protected:
    void Run() override;
    void Effect() override;

private:
    std::string filePath;

    std::atomic<int> result { 0 };
};

#endif // ASYNCTASK_PUSHSESSIONTASK_HPP
