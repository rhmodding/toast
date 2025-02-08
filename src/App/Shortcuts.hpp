#ifndef SHORTCUTS_HPP
#define SHORTCUTS_HPP

#include "Actions.hpp"

#if defined(__APPLE__)
    #define SCS_EXIT "Cmd+Q"
    #define SCST_CTRL "Cmd"
#else
    #define SCS_EXIT "Alt+F4"
    #define SCST_CTRL "Ctrl"
#endif // defined(__APPLE__)

#define SCS_LAUNCH_OPEN_SZS_DIALOG SCST_CTRL "+O"
#define SC_LAUNCH_OPEN_SZS_DIALOG (ImGuiKey_O | ImGuiMod_Ctrl)

#define SCS_LAUNCH_OPEN_TRADITIONAL_DIALOG SCST_CTRL "+Shift+O"
#define SC_LAUNCH_OPEN_TRADITIONAL_DIALOG (ImGuiKey_O | ImGuiMod_Ctrl | ImGuiMod_Shift)

#define SCS_SAVE_CURRENT_SESSION_SZS SCST_CTRL "+S"
#define SC_SAVE_CURRENT_SESSION_SZS (ImGuiKey_S | ImGuiMod_Ctrl)

#define SCS_LAUNCH_SAVE_AS_SZS_DIALOG SCST_CTRL "+Shift+S"
#define SC_LAUNCH_SAVE_AS_SZS_DIALOG (ImGuiKey_S | ImGuiMod_Ctrl | ImGuiMod_Shift)

#define SCS_UNDO SCST_CTRL "+Z"
#define SC_UNDO (ImGuiKey_Z | ImGuiMod_Ctrl)

#define SCS_REDO SCST_CTRL "+Shift+Z"
#define SC_REDO (ImGuiKey_Z | ImGuiMod_Ctrl | ImGuiMod_Shift)

namespace Shortcuts {

void Handle() {
    // ImGui::Shortcut must be called with flag ImGuiInputFlags_RouteAlways,
    // otherwise the input will not be registered

    if (ImGui::Shortcut(SC_LAUNCH_OPEN_SZS_DIALOG, ImGuiInputFlags_RouteAlways))
        return Actions::Dialog_CreateCompressedArcSession();
    else if (ImGui::Shortcut(SC_LAUNCH_OPEN_TRADITIONAL_DIALOG, ImGuiInputFlags_RouteAlways))
        return Actions::Dialog_CreateTraditionalSession();

    if (ImGui::Shortcut(SC_SAVE_CURRENT_SESSION_SZS, ImGuiInputFlags_RouteAlways))
        return Actions::SaveCurrentSessionSzs();
    else if (ImGui::Shortcut(SC_LAUNCH_SAVE_AS_SZS_DIALOG, ImGuiInputFlags_RouteAlways))
        return Actions::Dialog_SaveCurrentSessionAsSzs();

    SessionManager& sessionManager = SessionManager::getInstance();

    if (sessionManager.getSessionAvaliable()) {
        if (ImGui::Shortcut(SC_UNDO, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Repeat))
            return sessionManager.getCurrentSession()->undo();
        else if (ImGui::Shortcut(SC_REDO, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Repeat))
            return sessionManager.getCurrentSession()->redo();
    }
}

} // namespace Shortcuts

#endif // SHORTCUTS_HPP
