#ifndef ASYNCTASK_HPP
#define ASYNCTASK_HPP

#include <atomic>

#include "../common.hpp"

class AsyncTask {
public:
    AsyncTask(uint32_t id, const char* message);

    virtual ~AsyncTask() = default;

    void Start();

    // Run effect if this task is complete & the effect hasn't run already.
    void RunEffectIfComplete();

    void ShowPopup() const;

    bool getComplete() const { return this->isComplete; }
    bool getEffectRun() const { return this->hasEffectRun; }

protected:
    virtual void Run() = 0;
    virtual void Effect() = 0;

private:
    uint32_t id;

    std::atomic<bool> isComplete { false };
    std::atomic<bool> hasEffectRun { false };

    const char* message;

    ImGuiID imguiID;

    float startTime;
};

#endif // ASYNCTASK_HPP
