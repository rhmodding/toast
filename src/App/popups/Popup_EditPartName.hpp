#ifndef POPUP_EDITPARTNAME_HPP
#define POPUP_EDITPARTNAME_HPP

#include <imgui.h>

#include "../../SessionManager.hpp"
#include "../../command/CommandModifyArrangementPart.hpp"

#include "../../common.hpp"

static bool textInputStdString(const char* label, std::string& str) {
    constexpr ImGuiTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    return ImGui::InputText(label, const_cast<char*>(str.c_str()), str.capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) -> int {
        std::string* str = (std::string*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            // Resize string callback
            //
            // If for some reason we refuse the new length (BufTextLen) and/or
            // capacity (BufSize) we need to set them back to what we want.
            IM_ASSERT(data->Buf == str->c_str());

            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }

        return 0;
    }, &str);
}

static void Popup_EditPartName(int arrangementIndex, int partIndex) {
    if (arrangementIndex < 0 || partIndex < 0)
        return;

    static bool lateOpen { false };

    static std::string newName;

    CENTER_NEXT_WINDOW();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    bool open = ImGui::BeginPopupModal("Edit part editor name###EditPartName", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    SessionManager& sessionManager = SessionManager::getInstance();

    if (!lateOpen && open) {
        const CellAnim::ArrangementPart& part =
            sessionManager.getCurrentSession()
                ->getCurrentCellanim().object
                ->arrangements.at(arrangementIndex).parts.at(partIndex);

        if (!part.editorName.empty())
            newName = part.editorName;
        else
            newName[0] = '\0';
    }

    if (open) {
        ImGui::Text("Edit name for part no. %u (arrangement no. %u):", partIndex + 1, arrangementIndex + 1);

        textInputStdString("##Input", newName);

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("OK", { 120.f, 0.f })) {
            CellAnim::ArrangementPart newPart =
                sessionManager.getCurrentSession()
                ->getCurrentCellanim().object
                ->arrangements.at(arrangementIndex).parts.at(partIndex);

            newPart.editorName = newName;

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangementPart>(
                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                arrangementIndex, partIndex,
                newPart
            ));

            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", { 120.f, 0.f }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    lateOpen = open;
}

#endif // POPUP_EDITPARTNAME_HPP
