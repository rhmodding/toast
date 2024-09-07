#ifndef POPUP_CLOSEMODIFIEDSESSION
#define POPUP_CLOSEMODIFIEDSESSION

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../common.hpp"

void Popup_CloseModifiedSession() {
    GET_SESSION_MANAGER;

    if (sessionManager.sessionClosing < sessionManager.sessionList.size())
        return;

    const std::string& sessionPath =
        sessionManager.sessionList.at(sessionManager.sessionClosing).mainPath;

    char popupTitle[512];
    snprintf(
        popupTitle, 512 - 1,
        "%s###CloseModifiedSession",
        sessionPath.c_str()
    );

    CENTER_NEXT_WINDOW;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Dummy({ImGui::CalcTextSize(sessionPath.c_str()).x - 40, 0 });

        ImGui::Text("Are you sure you want to close this session?\nYour changes will be lost.");

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Close without saving")) {
            sessionManager.FreeSessionIndex(sessionManager.sessionClosing);
            sessionManager.SessionChanged();

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_CLOSEMODIFIEDSESSION
