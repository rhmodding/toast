#ifndef ASYNC_TASK_OPTIMIZECELLANIM_HPP
#define ASYNC_TASK_OPTIMIZECELLANIM_HPP

#include "AsyncTask.hpp"

#include "glInclude.hpp"

#include "Session.hpp"

struct OptimizeCellanimOptions {
    bool removeAnimationNames { false };
    bool removeDuplicateArrangements { true };
    bool removeUnusedArrangements { true };

    enum DownscaleOption {
        DownscaleOption_None,
        DownscaleOption_0_875x,
        DownscaleOption_0_75x,
        DownscaleOption_0_5x
    } downscaleSpritesheet { DownscaleOption_None };
};

class AsyncTaskOptimizeCellanim : public AsyncTask {
public:
    AsyncTaskOptimizeCellanim(
        AsyncTaskId id,
        Session* session, OptimizeCellanimOptions options
    );

protected:
    void Run() override;
    void Effect() override;

private:
    Session* mSession;
    OptimizeCellanimOptions mOptions;
};

#endif // ASYNC_TASK_OPTIMIZECELLANIM_HPP
