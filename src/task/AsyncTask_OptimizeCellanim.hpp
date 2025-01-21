#ifndef ASYNCTASK_OPTIMIZECELLANIM_HPP
#define ASYNCTASK_OPTIMIZECELLANIM_HPP

#include "AsyncTask.hpp"

#include <algorithm>

#include <unordered_set>

#include "../glInclude.hpp"

#include "../SessionManager.hpp"
#include "../MtCommandManager.hpp"

#include "../AppState.hpp"

struct OptimizeCellanimOptions {
    bool removeAnimationNames { false };
    bool removeUnusedArrangements { true };

    enum DownscaleOption {
        DownscaleOption_None,
        DownscaleOption_0_875x,
        DownscaleOption_0_75x,
        DownscaleOption_0_5x
    } downscaleSpritesheet { DownscaleOption_None };
};

class OptimizeCellanimTask : public AsyncTask {
public:
    OptimizeCellanimTask(
        uint32_t id,
        SessionManager::Session* session, OptimizeCellanimOptions options
    );

protected:
    void Run() override;
    void Effect() override;

private:
    SessionManager::Session* session;
    OptimizeCellanimOptions options;
};

#endif // ASYNCTASK_OPTIMIZECELLANIM_HPP
