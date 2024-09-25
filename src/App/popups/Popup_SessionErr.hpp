#ifndef POPUP_SESSIONERR
#define POPUP_SESSIONERR

#include <imgui.h>

#include "../../AppState.hpp"

#include "../../SessionManager.hpp"

#include "../../common.hpp"

void Popup_SessionErr() {
    SessionManager::SessionError errorCode = SessionManager::getInstance().lastSessionError;

    char errorMessage[256];
    unsigned errorType; // 0 = open, 1 = export, 2 = unknown
    switch (errorCode) {
        case SessionManager::SessionOpenError_FailOpenArchive:
            strcpy(errorMessage, "The archive file could not be opened.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_FailFindTPL:
            strcpy(errorMessage, "The archive does not contain the cellanim.tpl file.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_RootDirNotFound:
            strcpy(errorMessage, "The archive does not contain a root directory.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_NoBXCADsFound:
            strcpy(errorMessage, "The archive does not contain any brcad/bccad files.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_FailOpenBXCAD:
            // TODO: replace brcad with brcad/bccad when Megamix support is implemented
            strcpy(errorMessage, "The brcad file could not be opened.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_FailOpenTPL:
            strcpy(errorMessage, "The TPL file could not be opened.");
            errorType = 0;
            break; 
        case SessionManager::SessionOpenError_FailOpenPNG:
            strcpy(errorMessage, "The image file (.png) could not be opened.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_FailOpenHFile:
            strcpy(errorMessage, "The header file (.h) could not be opened.");
            errorType = 0;
            break;
        case SessionManager::SessionOpenError_SessionsFull:
            strcpy(errorMessage, "The maximum amount of sessions are already open.");
            errorType = 0;
            break;

        case SessionManager::SessionOutError_FailOpenFile:
            strcpy(errorMessage, "The destination file could not be opened for reading.");
            errorType = 1;
            break;
        case SessionManager::SessionOutError_ZlibError:
            strcpy(errorMessage, "Zlib raised an error while compressing the file.\nPlease check the log for more details.");
            errorType = 1;
            break;
        case SessionManager::SessionOutError_FailTPLTextureExport:
            strcpy(errorMessage, "There was an error exporting a texture.");
            errorType = 1;
            break;

        default:
            snprintf(errorMessage, 256, "An unknown error has occurred (code %i).", errorCode);
            errorType = 2;
            break;
    }

    char popupTitle[256];
    switch (errorType)  {
        case 0:
            snprintf(
                popupTitle, 256,
                "There was an error opening the session (code %i).###SessionErr",
                errorCode
            );
            break;
        case 1:
            snprintf(
                popupTitle, 256,
                "There was an error exporting the session (code %i).###SessionErr",
                errorCode
            );
            break;

        default:
            snprintf(
                popupTitle, 256,
                "There was an unknown session-related error (code %i).###SessionErr",
                errorCode
            );
            break;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });

    CENTER_NEXT_WINDOW;
    if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(errorMessage);

        ImGui::Dummy({ ImGui::CalcTextSize(popupTitle, nullptr, true).x - 40, 5});

        if (ImGui::Button("Alright", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

#endif // POPUP_SESSIONERR
