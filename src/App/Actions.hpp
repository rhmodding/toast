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
    const char* filterPatterns[] = { "*.szs" };
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
    char* pathList[3];

    const char* vFiltersBrcad[] { "*.brcad" };
    const char* tFiltersBrcad = "Binary Revolution CellAnim Data file (.brcad)";

    const char* vFiltersImage[]  { "*.png", "*.tga", "*.bmp", "*.psd", "*.jpg" };
    const char* tFiltersImage =  "Image files (.png, .tga, .bmp, .psd, .jpg)";

    const char* vFiltersHeader[] { "*.h" };
    const char* tFiltersHeader = "Header file (.h)";

    char* brcadDialog = tinyfd_openFileDialog(
        "Select the BRCAD file",
        nullptr,
        ARRAY_LENGTH(vFiltersBrcad), vFiltersBrcad,
        tFiltersBrcad,
        false
    );
    if (!brcadDialog)
        return;

    pathList[0] = new char[strlen(brcadDialog) + 1];
    strcpy(pathList[0], brcadDialog);

    char* imageDialog = tinyfd_openFileDialog(
        "Select the image file",
        nullptr,
        ARRAY_LENGTH(vFiltersImage), vFiltersImage,
        tFiltersImage,
        false
    );
    if (!imageDialog)
        return;

    pathList[1] = new char[strlen(imageDialog) + 1];
    strcpy(pathList[1], imageDialog);

    char* headerDialog = tinyfd_openFileDialog(
        "Select the header file",
        nullptr,
        ARRAY_LENGTH(vFiltersHeader), vFiltersHeader,
        tFiltersHeader,
        false
    );
    if (!headerDialog)
        return;

    pathList[2] = new char[strlen(headerDialog) + 1];
    strcpy(pathList[2], headerDialog);

    GET_SESSION_MANAGER;

    int32_t result = sessionManager.PushSessionTraditional((const char **)pathList);
    if (result < 0)
        AppState::getInstance().OpenGlobalPopup("###SessionErr");
    else {
        sessionManager.currentSession = result;
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

    const char* filterPatterns[] = { "*.szs" };
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
