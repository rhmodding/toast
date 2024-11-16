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

public:
    template <typename TaskType, typename... Args>
    void StartTask(Args&&... args) {
        auto task = std::make_unique<TaskType>(this->GenerateId(), std::forward<Args>(args)...);
        task->Start();

        this->tasks.emplace_back(std::move(task));
    }

    void UpdateTasks() {
        for (auto& task : this->tasks) {
            task->RenderPopup();
            task->TryRunEffect();
        }

        this->tasks.erase(
            std::remove_if(this->tasks.begin(), this->tasks.end(),
            [](const std::unique_ptr<AsyncTask>& task) {
                return task->IsComplete() && task->HasEffectRun();
            }),

            this->tasks.end()
        );
    }

    template <typename TaskType>
    bool HasTaskOfType() const {
        for (const auto& task : this->tasks) {
            if (dynamic_cast<TaskType*>(task.get()) != nullptr)
                return true;
        }

        return false;
    }

private:
    uint32_t GenerateId() {
        return this->nextId++;
    }

    std::atomic<uint32_t> nextId;
    std::vector<std::unique_ptr<AsyncTask>> tasks;

private:
    AsyncTaskManager() :
        nextId(0)
    {}
};

#endif // ASYNCTASKMANAGER_HPP
