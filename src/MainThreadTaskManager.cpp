#include "MainThreadTaskManager.hpp"

std::future<void> MainThreadTaskManager::enqueueCommand(std::function<void()> func) {
    if (std::this_thread::get_id() == gAppPtr->getMainThreadId()) {
        func();

        std::promise<void> promise;
        promise.set_value();

        return promise.get_future();
    }

    std::lock_guard<std::mutex> lock(this->mtx);

    MainThreadTask command { func, std::promise<void>() };
    std::future<void> future = command.promise.get_future();

    this->commandQueue.push(std::move(command));

    this->queueCondition.notify_one();

    return future;
}

void MainThreadTaskManager::Update() {
    std::unique_lock<std::mutex> lock(this->mtx);
    while (!this->commandQueue.empty()) {
        auto command = std::move(this->commandQueue.front());
        this->commandQueue.pop();

        lock.unlock();
            command.func();

            command.promise.set_value();
        lock.lock();
    }
}
