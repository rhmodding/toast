#include "MainThreadTaskManager.hpp"

#include "Toast.hpp"

std::future<void> MainThreadTaskManager::queueTask(std::function<void()> func) {
    // If we're already on the main thread, run the task now
    if (std::this_thread::get_id() == Toast::getInstance()->getMainThreadId()) {
        func();

        std::promise<void> promise;
        promise.set_value();

        return promise.get_future();
    }

    std::lock_guard<std::mutex> lock(mMtx);

    MainThreadTask task {
        .func = func,
        .promise = std::promise<void>()
    };
    std::future<void> future = task.promise.get_future();

    mTaskQueue.push(std::move(task));

    mQueueCondition.notify_one();

    return future;
}

void MainThreadTaskManager::update() {
    std::unique_lock<std::mutex> lock(mMtx);

    while (!mTaskQueue.empty()) {
        MainThreadTask task = std::move(mTaskQueue.front());
        mTaskQueue.pop();

        lock.unlock();
            task.func();
            task.promise.set_value();
        lock.lock();
    }
}
