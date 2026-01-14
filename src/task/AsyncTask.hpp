#ifndef ASYNC_TASK_HPP
#define ASYNC_TASK_HPP

#include <imgui.h>

#include <atomic>

#include <cstdint>

typedef uint32_t AsyncTaskId;

class AsyncTask {
public:
    AsyncTask(AsyncTaskId id, const char* message);
    virtual ~AsyncTask() = default;

    void start();

    void update();

    void showPopup() const;

    AsyncTaskId getId() const { return mId; }
    bool getIsComplete() const { return mIsComplete; }
    bool getHasEffectRun() const { return mHasEffectRun; }

protected:
    virtual void run() = 0;
    virtual void effect() = 0;

private:
    AsyncTaskId mId;

    std::atomic<bool> mIsComplete { false };
    std::atomic<bool> mHasEffectRun { false };

    const char *mMessage;

    float mStartTime;
};

#endif // ASYNC_TASK_HPP
