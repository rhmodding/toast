#ifndef MainThreadTaskManager_HPP
#define MainThreadTaskManager_HPP

#include "Singleton.hpp"

#include <mutex>
#include <condition_variable>

#include <functional>

#include <future>

#include <queue>

class MainThreadTaskManager : public Singleton<MainThreadTaskManager> {
    friend class Singleton<MainThreadTaskManager>;

private:
    MainThreadTaskManager() = default;
public:
    ~MainThreadTaskManager() = default;

public:
    struct MainThreadTask {
        std::function<void()> func;
        std::promise<void> promise;
    };

public:
    std::future<void> QueueTask(std::function<void()> func);

    void Update();

private:
    std::queue<MainThreadTask> mTaskQueue;
    std::condition_variable mQueueCondition;

    std::mutex mMtx;
};

#endif // MainThreadTaskManager_HPP
