#include "AsyncTaskManager.hpp"

#include <algorithm>

void AsyncTaskManager::update() {
    for (auto& task : mTasks) {
        task->update();
        task->showPopup();
    }

    mTasks.erase(
        std::remove_if(mTasks.begin(), mTasks.end(),
        [](const std::unique_ptr<AsyncTask>& task) {
            return task->getIsComplete() && task->getHasEffectRun();
        }),

        mTasks.end()
    );
}
