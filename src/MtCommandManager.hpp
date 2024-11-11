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

    std::future<void> enqueueCommand(std::function<void()> func);

    // Needs to run every cycle on the main thread
    void Update();
private:
    MtCommandManager() {} // Private constructor to prevent instantiation
};

#endif // MTCOMMANDMANAGER_HPP
