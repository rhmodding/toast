#ifndef MAIN_THREAD_TASK_MANAGER_HPP
#define MAIN_THREAD_TASK_MANAGER_HPP

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
    std::future<void> queueTask(std::function<void()> func);

    void update();

private:
    std::queue<MainThreadTask> mTaskQueue;
    std::condition_variable mQueueCondition;

    std::mutex mMtx;
};

#endif // MAIN_THREAD_TASK_MANAGER_HPP
