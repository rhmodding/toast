#include "AsyncTaskManager.hpp"

#include <algorithm>

void AsyncTaskManager::UpdateTasks() {
    for (auto& task : mTasks) {
        task->ShowPopup();
        task->RunEffectIfComplete();
    }

    mTasks.erase(
        std::remove_if(mTasks.begin(), mTasks.end(),
        [](const std::unique_ptr<AsyncTask>& task) {
            return task->getIsComplete() && task->getHasEffectRun();
        }),

        mTasks.end()
    );
}
