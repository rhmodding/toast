#ifndef ASYNC_TASK_MANAGER_HPP
#define ASYNC_TASK_MANAGER_HPP

#include "../Singleton.hpp"

#include "AsyncTask.hpp"

#include <vector>

#include <memory>
#include <atomic>

class AsyncTaskManager : public Singleton<AsyncTaskManager> {
    friend class Singleton<AsyncTaskManager>;

private:
    AsyncTaskManager() = default;
public:
    ~AsyncTaskManager() = default;

public:
    template <typename TaskType, typename... Args>
    AsyncTaskId StartTask(Args&&... args);

    void UpdateTasks();

    template <typename TaskType>
    bool hasTaskOfType() const;

private:
    std::atomic<AsyncTaskId> mNextId { 0 };
    std::vector<std::unique_ptr<AsyncTask>> mTasks;
};

template <typename TaskType, typename... Args>
AsyncTaskId AsyncTaskManager::StartTask(Args&&... args) {
    AsyncTaskId id = mNextId++;

    auto task = std::make_unique<TaskType>(id, std::forward<Args>(args)...);
    task->Start();

    mTasks.emplace_back(std::move(task));

    return id;
}

template <typename TaskType>
bool AsyncTaskManager::hasTaskOfType() const {
    for (const auto& task : mTasks) {
        if (dynamic_cast<TaskType*>(task.get()) != nullptr)
            return true;
    }

    return false;
}

#endif // ASYNC_TASK_MANAGER_HPP
