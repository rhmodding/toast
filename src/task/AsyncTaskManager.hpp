#ifndef ASYNCTASKMANAGER_HPP
#define ASYNCTASKMANAGER_HPP

#include "../Singleton.hpp"

#include "AsyncTask.hpp"

#include <vector>

#include <memory>
#include <atomic>

// Stores instance of AsyncTaskManager in local asyncTaskManager.
#define GET_ASYNC_TASK_MANAGER AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance()

class AsyncTaskManager : public Singleton<AsyncTaskManager> {
    friend class Singleton<AsyncTaskManager>; // Allow access to base class constructor

private:
    AsyncTaskManager() = default;
public:
    ~AsyncTaskManager() = default;

public:
    template <typename TaskType, typename... Args>
    void StartTask(Args&&... args);

    void UpdateTasks();

    template <typename TaskType>
    bool HasTaskOfType() const;

private:
    std::atomic<uint32_t> nextId { 0 };
    std::vector<std::unique_ptr<AsyncTask>> tasks;
};

template <typename TaskType, typename... Args>
void AsyncTaskManager::StartTask(Args&&... args) {
    auto task = std::make_unique<TaskType>(this->nextId++, std::forward<Args>(args)...);
    task->Start();
    this->tasks.emplace_back(std::move(task));
}

template <typename TaskType>
bool AsyncTaskManager::HasTaskOfType() const {
    for (const auto& task : this->tasks) {
        if (dynamic_cast<TaskType*>(task.get()) != nullptr)
            return true;
    }

    return false;
}

#endif // ASYNCTASKMANAGER_HPP
