#ifndef SHORTCUTS_HPP
#define SHORTCUTS_HPP

#include "Actions.hpp"

#if defined(__APPLE__)
    #define SCS_EXIT "Cmd+Q"
    #define SCST_CTRL "Cmd"
#else
    #define SCS_EXIT "Alt+F4"
    #define SCST_CTRL "Ctrl"
#endif

#define SCT_CTRL ImGui::GetIO().KeyCtrl

#define SCS_LAUNCH_OPEN_SZS_DIALOG SCST_CTRL "+O"
#define SC_LAUNCH_OPEN_SZS_DIALOG \
            SCT_CTRL && \
            !ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_O)

#define SCS_LAUNCH_OPEN_TRADITIONAL_DIALOG SCST_CTRL "+Shift+O"
#define SC_LAUNCH_OPEN_TRADITIONAL_DIALOG \
            SCT_CTRL && \
            ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_O)

#define SCS_SAVE_CURRENT_SESSION_SZS SCST_CTRL "+S"
#define SC_SAVE_CURRENT_SESSION_SZS \
            SCT_CTRL && \
            !ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_S)

#define SCS_LAUNCH_SAVE_AS_SZS_DIALOG SCST_CTRL "+Shift+S"
#define SC_LAUNCH_SAVE_AS_SZS_DIALOG \
            SCT_CTRL && \
            ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_S)

#define SCS_UNDO SCST_CTRL "+Z"
#define SC_UNDO \
            SCT_CTRL && \
            !ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_Z)

#define SCS_REDO SCST_CTRL "+Shift+Z"
#define SC_REDO \
            SCT_CTRL && \
            ImGui::GetIO().KeyShift && \
            ImGui::IsKeyReleased(ImGuiKey_Z)

namespace Shortcuts {

void Handle() {
    if (SC_LAUNCH_OPEN_SZS_DIALOG)
        Actions::Dialog_CreateCompressedArcSession();
    if (SC_LAUNCH_OPEN_TRADITIONAL_DIALOG)
        Actions::Dialog_CreateTraditionalSession();
    if (SC_SAVE_CURRENT_SESSION_SZS)
        Actions::SaveCurrentSessionSzs();
    if (SC_LAUNCH_SAVE_AS_SZS_DIALOG)
        Actions::Dialog_SaveCurrentSessionAsSzs();

    GET_SESSION_MANAGER;

    if (sessionManager.getSessionAvaliable()) {
        if (SC_UNDO)
            sessionManager.getCurrentSession()->undo();
        else if (SC_REDO)
            sessionManager.getCurrentSession()->redo();
    }
}

} // namespace Shortcuts

#endif // SHORTCUTS_HPP
