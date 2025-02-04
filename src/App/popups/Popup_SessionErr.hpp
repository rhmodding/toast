#ifndef POPUP_SESSIONERR_HPP
#define POPUP_SESSIONERR_HPP

#include <imgui.h>

#include "../../AppState.hpp"

#include "../../SessionManager.hpp"

#include "../../common.hpp"

static void Popup_SessionErr() {
    auto error = SessionManager::getInstance().currentError;

    char errorMessage[256];
    unsigned errorType { 2 }; // 0 = open, 1 = export, 2 = unknown
    switch (error) {
    case SessionManager::OpenError_FileDoesNotExist:
        strcpy(errorMessage, "The file (.szs) could not be opened because it does not exist.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenArchive:
        strcpy(errorMessage, "The archive could not be opened. Is this a cellanim file?");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailFindTPL:
        strcpy(errorMessage, "The archive does not contain the cellanim.tpl file.");
        errorType = 0;
        break;
    case SessionManager::OpenError_RootDirNotFound:
        strcpy(errorMessage, "The archive does not contain a root directory.");
        errorType = 0;
        break;
    case SessionManager::OpenError_NoBXCADsFound:
        strcpy(errorMessage, "The archive does not contain any brcad/bccad files.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenBXCAD:
        // TODO: replace brcad with brcad/bccad when Megamix support is implemented
        strcpy(errorMessage, "The cellanim file (.brcad) could not be opened.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenTPL:
        strcpy(errorMessage, "The TPL file could not be opened.");
        errorType = 0;
        break; 
    case SessionManager::OpenError_FailOpenImage:
        strcpy(errorMessage, "The image file could not be opened.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenHFile:
        strcpy(errorMessage, "The header file (.h) could not be opened.");
        errorType = 0;
        break;
    case SessionManager::OpenError_LayoutArchive:
        strcpy(errorMessage, "The selected archive is a layout archive. Please choose a\ncellanim archive instead (cellanim.szs).");
        errorType = 0;
        break;
    case SessionManager::OpenError_SessionsFull:
        strcpy(errorMessage, "The maximum amount of sessions are already open.");
        errorType = 0;
        break;

    case SessionManager::OutError_FailOpenFile:
        strcpy(errorMessage, "The destination file could not be opened for reading.");
        errorType = 1;
        break;
    case SessionManager::OutError_ZlibError:
        strcpy(errorMessage, "Zlib raised an error while compressing the file.\nPlease check the log for more details.");
        errorType = 1;
        break;
    case SessionManager::OutError_FailTPLTextureExport:
        strcpy(errorMessage, "There was an error exporting a texture.");
        errorType = 1;
        break;

    default:
        snprintf(
            errorMessage, sizeof(errorMessage),
            "An unknown error has occurred (code %d).", (int)error
        );
        errorType = 2;
        break;
    }

    char popupTitle[256];
    switch (errorType)  {
    case 0:
        snprintf(
            popupTitle, sizeof(popupTitle),
            "There was an error opening the session (code %d).###SessionErr",
            (int)error
        );
        break;
    case 1:
        snprintf(
            popupTitle, sizeof(popupTitle),
            "There was an error exporting the session (code %d).###SessionErr",
            (int)error
        );
        break;

    default:
        snprintf(
            popupTitle, sizeof(popupTitle),
            "There was an unknown session-related error (code %d).###SessionErr",
            (int)error
        );
        break;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 15.f });

    CENTER_NEXT_WINDOW;
    if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(errorMessage);

        ImGui::Dummy({ ImGui::CalcTextSize(popupTitle, nullptr, true).x - 40.f, 5.f });

        if (ImGui::Button("Alright", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::Dummy({ 0.f, 5.f });

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

#endif // POPUP_SESSIONERR_HPP
