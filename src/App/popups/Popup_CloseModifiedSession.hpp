#ifndef POPUP_CLOSEMODIFIEDSESSION_HPP
#define POPUP_CLOSEMODIFIEDSESSION_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"

#include "../../Macro.hpp"

static void Popup_CloseModifiedSession() {
    SessionManager& sessionManager = SessionManager::getInstance();

    if (
        sessionManager.getSessionClosingIndex() < 0 || 
        sessionManager.getSessionClosingIndex() >= static_cast<int>(sessionManager.getSessionCount())
    )
        return;

    const std::string& sessionPath =
        sessionManager.getSession(sessionManager.getSessionClosingIndex()).resourcePath;

    char popupTitle[512];
    snprintf(
        popupTitle, sizeof(popupTitle),
        "%s###CloseModifiedSession",
        sessionPath.c_str()
    );

    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Dummy({ ImGui::CalcTextSize(sessionPath.c_str()).x - 40.f, 0.f });

        ImGui::TextUnformatted("Are you sure you want to close this session?\nYour changes will be lost.");

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Close without saving")) {
            sessionManager.RemoveSession(sessionManager.getSessionClosingIndex());

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

#endif // POPUP_CLOSEMODIFIEDSESSION_HPP
