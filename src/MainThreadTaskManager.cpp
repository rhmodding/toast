#include "MainThreadTaskManager.hpp"

#include "Toast.hpp"

std::future<void> MainThreadTaskManager::QueueTask(std::function<void()> func) {
    // If we're already on the main thread, run the task now
    if (std::this_thread::get_id() == Toast::getInstance()->getMainThreadId()) {
        func();

        std::promise<void> promise;
        promise.set_value();

        return promise.get_future();
    }

    std::lock_guard<std::mutex> lock(this->mtx);

    MainThreadTask task {
        .func = func,
        .promise = std::promise<void>()
    };
    std::future<void> future = task.promise.get_future();

    this->taskQueue.push(std::move(task));

    this->queueCondition.notify_one();

    return future;
}

void MainThreadTaskManager::Update() {
    std::unique_lock<std::mutex> lock(this->mtx);

    while (!this->taskQueue.empty()) {
        MainThreadTask task = std::move(this->taskQueue.front());
        this->taskQueue.pop();

        lock.unlock();
            task.func();
            task.promise.set_value();
        lock.lock();
    }
}
