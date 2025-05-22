#ifndef ASYNCTASK_HPP
#define ASYNCTASK_HPP

#include <imgui.h>

#include <atomic>

#include <cstdint>

typedef uint32_t AsyncTaskId;

class AsyncTask {
public:
    AsyncTask(AsyncTaskId id, const char* message);
    virtual ~AsyncTask() = default;

    void Start();

    // Run effect if this task is complete & the effect hasn't run already.
    void RunEffectIfComplete();

    void ShowPopup() const;

    AsyncTaskId getId() const { return mId; }
    bool getIsComplete() const { return mIsComplete; }
    bool getHasEffectRun() const { return mHasEffectRun; }

protected:
    virtual void Run() = 0;
    virtual void Effect() = 0;

private:
    AsyncTaskId mId;

    std::atomic<bool> mIsComplete { false };
    std::atomic<bool> mHasEffectRun { false };

    const char* mMessage;

    float mStartTime;
};

#endif // ASYNCTASK_HPP
