#ifndef ACTIONS_HPP
#define ACTIONS_HPP

#include <tinyfiledialogs.h>

#include <imgui.h>

#include "../SessionManager.hpp"

#include "../task/AsyncTaskManager.hpp"
#include "../task/AsyncTask_PushSessionTask.hpp"
#include "../task/AsyncTask_ExportSessionTask.hpp"

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

    AsyncTaskManager::getInstance().StartTask<PushSessionTask>(
        std::string(openFileDialog)
    );
}

void Dialog_CreateTraditionalSession() {
    char* brcadPath, *imagePath, *headerPath;

    const char* vFiltersBrcad[] { "*.brcad" };
    const char* tFiltersBrcad = "Binary Revolution CellAnim Data file (.brcad)";

    const char* vFiltersImage[] { "*.png", "*.tga", "*.bmp", "*.psd", "*.jpg" };
    const char* tFiltersImage = "Image file (.png, .tga, .bmp, .psd, .jpg)";

    const char* vFiltersHeader[] { "*.h" };
    const char* tFiltersHeader = "Header file (.h)";

    char* tempStr;

    tempStr = tinyfd_openFileDialog(
        "Select the BRCAD file",
        nullptr,
        ARRAY_LENGTH(vFiltersBrcad), vFiltersBrcad,
        tFiltersBrcad,
        false
    );
    if (!tempStr)
        return;

    brcadPath = strdup(tempStr);

    tempStr = tinyfd_openFileDialog(
        "Select the image file",
        nullptr,
        ARRAY_LENGTH(vFiltersImage), vFiltersImage,
        tFiltersImage,
        false
    );
    if (!tempStr)
        return;

    imagePath = strdup(tempStr);

    tempStr = tinyfd_openFileDialog(
        "Select the header file",
        nullptr,
        ARRAY_LENGTH(vFiltersHeader), vFiltersHeader,
        tFiltersHeader,
        false
    );
    if (!tempStr)
        return;

    headerPath = strdup(tempStr);

    GET_SESSION_MANAGER;

    // TODO: separate this into Task

    int result = sessionManager.PushSessionTraditional(brcadPath, imagePath, headerPath);
    if (result < 0)
        AppState::getInstance().OpenGlobalPopup("###SessionErr");
    else {
        sessionManager.currentSessionIndex = result;
        sessionManager.SessionChanged();
    }
}

void Dialog_SaveCurrentSessionAsSzs() {
    GET_SESSION_MANAGER;
    GET_ASYNC_TASK_MANAGER;

    if (
        !sessionManager.getSessionAvaliable() ||
        asyncTaskManager.HasTaskOfType<ExportSessionTask>()
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
        asyncTaskManager.StartTask<ExportSessionTask>(
            sessionManager.getCurrentSession(),
            std::string(saveFileDialog)
        );
}

void SaveCurrentSessionSzs() {
    GET_SESSION_MANAGER;
    GET_ASYNC_TASK_MANAGER;

    if (
        !sessionManager.getSessionAvaliable() ||
        sessionManager.getCurrentSession()->traditionalMethod ||

        asyncTaskManager.HasTaskOfType<ExportSessionTask>()
    )
        return;

    asyncTaskManager.StartTask<ExportSessionTask>(
        sessionManager.getCurrentSession(),
        sessionManager.getCurrentSession()->mainPath
    );
}

} // namespace Actions

#endif // ACTIONS_HPP
