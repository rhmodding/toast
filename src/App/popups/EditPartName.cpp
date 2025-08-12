#include "EditPartName.hpp"

#include <imgui.h>

#include "manager/SessionManager.hpp"

#include "command/CommandModifyArrangementPart.hpp"

#include "util/UIUtil.hpp"

#include "Macro.hpp"

void Popups::EditPartName::Update() {
    if (arrangementIndex < 0 || partIndex < 0)
        return;

    static bool lateOpen { false };

    static std::string newName;

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    bool open = ImGui::BeginPopupModal("Edit part editor name###EditPartName", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();

    SessionManager& sessionManager = SessionManager::getInstance();

    if (!lateOpen && open) {
        const CellAnim::ArrangementPart& part =
            sessionManager.getCurrentSession()
                ->getCurrentCellAnim().object
                ->getArrangement(arrangementIndex).parts.at(partIndex);

        if (!part.editorName.empty())
            newName = part.editorName;
        else
            newName[0] = '\0';
    }

    if (open) {
        ImGui::Text("Edit name for part no. %u (arrangement no. %u):", partIndex + 1, arrangementIndex + 1);

        UIUtil::Widget::StdStringTextInput("##Input", newName);

        ImGui::Dummy({ 0.f, 15.f });
        ImGui::Separator();
        ImGui::Dummy({ 0.f, 5.f });

        if (ImGui::Button("OK", { 120.f, 0.f })) {
            CellAnim::ArrangementPart newPart =
                sessionManager.getCurrentSession()
                ->getCurrentCellAnim().object
                ->getArrangement(arrangementIndex).parts.at(partIndex);

            newPart.editorName = newName;

            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandModifyArrangementPart>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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