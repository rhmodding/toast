#ifndef MainThreadTaskManager_HPP
#define MainThreadTaskManager_HPP

/*
    * MainThreadTaskManager
    *
    * MainThreadTaskManager enables threads to queue 'commands' on the main thread and await the result.
    * If MainThreadTaskManager::enqueueCommand is called on the main thread, the passed command will
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

// Stores instance of MainThreadTaskManager in local MainThreadTaskManager.
#define GET_MT_COMMAND_MANAGER MainThreadTaskManager& MainThreadTaskManager = MainThreadTaskManager::getInstance()

class MainThreadTaskManager : public Singleton<MainThreadTaskManager> {
    friend class Singleton<MainThreadTaskManager>; // Allow access to base class constructor

private:
    MainThreadTaskManager() = default;
public:
    ~MainThreadTaskManager() = default;

private:
    std::mutex mtx;
    std::condition_variable queueCondition;
public:
    struct MainThreadTask {
        std::function<void()> func;
        std::promise<void> promise;
    };
    std::queue<MainThreadTask> commandQueue;

    std::future<void> enqueueCommand(std::function<void()> func);

    // Needs to run every cycle on the main thread
    void Update();
};

#endif // MainThreadTaskManager_HPP
