#include "MtCommandManager.hpp"

std::future<void> MtCommandManager::enqueueCommand(std::function<void()> func) {
    if (
        std::this_thread::get_id() == gAppPtr->getMainThreadId() ||
        this->isDestroyed
    ) {
        func();

        std::promise<void> promise;
        promise.set_value();

        return promise.get_future();
    }

    std::lock_guard<std::mutex> lock(this->mtx);

    MtCommand command { func, std::promise<void>() };
    auto future = command.promise.get_future();

    this->commandQueue.push(std::move(command));

    this->queueCondition.notify_one();

    return future;
}

void MtCommandManager::Update() {
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