#ifndef ACTIONS_HPP
#define ACTIONS_HPP

#include <tinyfiledialogs.h>

#include <imgui.h>

#include "../SessionManager.hpp"

#include "../task/AsyncTaskManager.hpp"
#include "../task/AsyncTaskPushSession.hpp"
#include "../task/AsyncTaskExportSession.hpp"

#include <string>

#include "../common.hpp"

namespace Actions {

void CreateSessionPromptPath() {
    const char* filterPatterns[] { "*.szs", "*.zlib" };
    char* openFileDialog = tinyfd_openFileDialog(
        "Select a cellanim file",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Cellanim Archive Files",
        false
    );

    if (!openFileDialog)
        return;

    AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(
        std::string(openFileDialog)
    );
}

void ExportSessionPromptPath() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance();

    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>()
    )
        return;

    const bool isRvl = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_RVL;

    const char* filterPatterns[] { isRvl ? "*.szs" : "*.zlib" };
    char* saveFileDialog = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Cellanim Archive files"
    );

    if (saveFileDialog)
        asyncTaskManager.StartTask<AsyncTaskExportSession>(
            sessionManager.getCurrentSessionIndex(),
            std::string(saveFileDialog)
        );
}

void ExportSession() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance();

    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>()
    )
        return;

    asyncTaskManager.StartTask<AsyncTaskExportSession>(
        sessionManager.getCurrentSessionIndex()
    );
}

} // namespace Actions

#endif // ACTIONS_HPP
