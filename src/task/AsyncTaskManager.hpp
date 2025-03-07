#ifndef ASYNCTASKMANAGER_HPP
#define ASYNCTASKMANAGER_HPP

#include "../Singleton.hpp"

#include "AsyncTask.hpp"

#include <vector>

#include <memory>
#include <atomic>

class AsyncTaskManager : public Singleton<AsyncTaskManager> {
    friend class Singleton<AsyncTaskManager>; // Allow access to base class constructor

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
    std::atomic<AsyncTaskId> nextId { 0 };
    std::vector<std::unique_ptr<AsyncTask>> tasks;
};

template <typename TaskType, typename... Args>
AsyncTaskId AsyncTaskManager::StartTask(Args&&... args) {
    AsyncTaskId id = this->nextId++;

    auto task = std::make_unique<TaskType>(id, std::forward<Args>(args)...);
    task->Start();

    this->tasks.emplace_back(std::move(task));

    return id;
}

template <typename TaskType>
bool AsyncTaskManager::hasTaskOfType() const {
    for (const auto& task : this->tasks) {
        if (dynamic_cast<TaskType*>(task.get()) != nullptr)
            return true;
    }

    return false;
}

#endif // ASYNCTASKMANAGER_HPP
