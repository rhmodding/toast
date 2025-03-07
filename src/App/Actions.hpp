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

void Dialog_CreateCompressedArcSession() {
    const char* filterPatterns[] { "*.szs" };
    char* openFileDialog = tinyfd_openFileDialog(
        "Select a cellanim file",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Compressed Arc File (.szs)",
        false
    );

    if (!openFileDialog)
        return;

    AsyncTaskManager::getInstance().StartTask<AsyncTaskPushSession>(
        std::string(openFileDialog)
    );
}

void Dialog_SaveCurrentSessionAsSzs() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AsyncTaskManager& asyncTaskManager = AsyncTaskManager::getInstance();

    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.hasTaskOfType<AsyncTaskExportSession>()
    )
        return;

    const char* filterPatterns[] { "*.szs" };
    char* saveFileDialog = tinyfd_saveFileDialog(
        "Select a file to save to",
        nullptr,
        ARRAY_LENGTH(filterPatterns), filterPatterns,
        "Compressed Arc File (.szs)"
    );

    if (saveFileDialog)
        asyncTaskManager.StartTask<AsyncTaskExportSession>(
            sessionManager.getCurrentSessionIndex(),
            std::string(saveFileDialog)
        );
}

void SaveCurrentSessionSzs() {
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
