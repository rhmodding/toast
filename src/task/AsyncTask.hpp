#ifndef ASYNCTASK_HPP
#define ASYNCTASK_HPP

#include <imgui.h>

#include <atomic>

#include "../common.hpp"

typedef uint32_t AsyncTaskId;

class AsyncTask {
public:
    AsyncTask(AsyncTaskId id, const char* message);
    virtual ~AsyncTask() = default;

    void Start();

    // Run effect if this task is complete & the effect hasn't run already.
    void RunEffectIfComplete();

    void ShowPopup() const;

    AsyncTaskId getId() const { return this->id; }

    bool getComplete() const { return this->isComplete; }
    bool getEffectRun() const { return this->hasEffectRun; }

protected:
    virtual void Run() = 0;
    virtual void Effect() = 0;

private:
    AsyncTaskId id;

    std::atomic<bool> isComplete { false };
    std::atomic<bool> hasEffectRun { false };

    const char* message;

    float startTime;
};

#endif // ASYNCTASK_HPP
