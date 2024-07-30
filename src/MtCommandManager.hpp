#ifndef MTCOMMANDMANAGER_HPP
#define MTCOMMANDMANAGER_HPP

#include "Singleton.hpp"

#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

#include <queue>

#include "App.hpp"

// Stores instance of MtCommandManager in local mtCommandManager.
#define GET_MT_COMMAND_MANAGER MtCommandManager& mtCommandManager = MtCommandManager::getInstance()

class MtCommandManager : public Singleton<MtCommandManager> {
    friend class Singleton<MtCommandManager>; // Allow access to base class constructor

private:
    std::mutex mtx;
    std::condition_variable queueCondition;
public:
    struct MtCommand {
        std::function<void()> func;
        std::promise<void> promise;
    };
    std::queue<MtCommand> commandQueue;

    std::future<void> enqueueCommand(std::function<void()> func) {
        if (std::this_thread::get_id() == gAppPtr->getWindowThreadID()) {
            func();

            std::promise<void> promise;
            promise.set_value();

            return promise.get_future();
        }

        std::lock_guard<std::mutex> lock(this->mtx);

        MtCommand command{ func, std::promise<void>() };
        auto future = command.promise.get_future();

        this->commandQueue.push(std::move(command));

        this->queueCondition.notify_one();

        return future;
    }

    // Needs to run every cycle on the main thread
    void Update() {
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

private:
    MtCommandManager() {} // Private constructor to prevent instantiation
};

#endif // MTCOMMANDMANAGER_HPP
