#ifndef MTCOMMANDMANAGER_HPP
#define MTCOMMANDMANAGER_HPP

/*
    * MtCommandManager
    *
    * MtCommandManager enables threads to queue 'commands' on the main thread and await the result.
    * If MtCommandManager::enqueueCommand is called on the main thread, the passed command will
    * be executed immediately.
    * 
    * This is primarily used for GL calls, which will no-op if invoked outside of the main thread.
*/

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
    MtCommandManager() = default;
public:
    ~MtCommandManager() = default;

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
};

#endif // MTCOMMANDMANAGER_HPP
