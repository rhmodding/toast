#include "AsyncTaskManager.hpp"

void AsyncTaskManager::UpdateTasks() {
    for (auto& task : this->tasks) {
        task->ShowPopup();
        task->RunEffectIfComplete();
    }

    this->tasks.erase(
        std::remove_if(this->tasks.begin(), this->tasks.end(),
        [](const std::unique_ptr<AsyncTask>& task) {
            return task->getComplete() && task->getEffectRun();
        }),

        this->tasks.end()
    );
}
