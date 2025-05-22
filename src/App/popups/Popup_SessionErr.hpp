#ifndef POPUP_SESSIONERR_HPP
#define POPUP_SESSIONERR_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../common.hpp"

static void Popup_SessionErr() {
    SessionManager::Error error = SessionManager::getInstance().getCurrentError();

    char errorMessage[256];
    unsigned errorType { 2 }; // 0 = open, 1 = export, 2 = unknown

    switch (error) {
    case SessionManager::OpenError_FileDoesNotExist:
        strcpy(errorMessage, "The specified file could not be opened because it does not exist.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenFile:
        strcpy(errorMessage, "The specified file could not be opened.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenArchive:
        strcpy(errorMessage, "The archive could not be opened. Is this a cellanim archive?");
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
        strcpy(errorMessage, "The archive does not contain any cellanim files (.brcad / .bccad).");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenBXCAD:
        strcpy(errorMessage, "The cellanim file (.brcad / .bccad) could not be opened. It might be corrupted.");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenTPL:
        strcpy(errorMessage, "The TPL file could not be opened. It might be corrupted.");
        errorType = 0;
        break;
    case SessionManager::OpenError_MissingCTPK:
        strcpy(errorMessage, "A CTPK file is missing (not all cellanim files could be matched with a CTPK file).");
        errorType = 0;
        break;
    case SessionManager::OpenError_FailOpenCTPK:
        strcpy(errorMessage, "A CTPK file could not be opened. It might be corrupted.");
        errorType = 0;
        break;
    case SessionManager::OpenError_NoCTPKTextures:
        strcpy(errorMessage, "A CTPK file has no valid textures. It might be corrupted.");
        errorType = 0;
        break;
    case SessionManager::OpenError_LayoutArchive:
        strcpy(errorMessage,
            "The selected file is a layout archive. Please choose a\n"
            "cellanim archive instead."
        );
        errorType = 0;
        break;
    case SessionManager::OpenError_EffectResource:
        strcpy(errorMessage,
            "The selected file is an effect file. Please choose a\n"
            "cellanim archive instead."
        );
        errorType = 0;
        break;
    case SessionManager::OpenError_BCRES:
        strcpy(errorMessage,
            "The selected file is a BCRES. Please choose a\n"
            "cellanim archive instead."
        );
        errorType = 0;
        break;


    case SessionManager::OutError_FailOpenFile:
        strcpy(errorMessage, "The destination file could not be opened for reading.");
        errorType = 1;
        break;
    case SessionManager::OutError_FailBackupFile:
        strcpy(errorMessage,
            "The destination file already exists and could not be backed up (does toast have the\n"
            "proper permissions to access this file?)\n"
            "You can disable backup behaviour in the config menu."
        );
        errorType = 1;
        break;
    case SessionManager::OutError_ZlibError:
        strcpy(errorMessage, "Zlib raised an error while compressing the file.\nPlease check the log for more details.");
        errorType = 1;
        break;
    case SessionManager::OutError_FailTextureExport:
        strcpy(errorMessage, "There was an error exporting a texture.");
        errorType = 1;
        break;

    default:
        snprintf(
            errorMessage, sizeof(errorMessage),
            "An unknown error has occurred (code %d).",
            static_cast<int>(error)
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
            static_cast<int>(error)
        );
        break;
    case 1:
        snprintf(
            popupTitle, sizeof(popupTitle),
            "There was an error exporting the session (code %d).###SessionErr",
            static_cast<int>(error)
        );
        break;

    default:
        snprintf(
            popupTitle, sizeof(popupTitle),
            "There was an unknown session-related error (code %d).###SessionErr",
            static_cast<int>(error)
        );
        break;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 15.f });

    CENTER_NEXT_WINDOW();
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
