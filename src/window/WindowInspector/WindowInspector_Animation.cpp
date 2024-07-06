#include "../WindowInspector.hpp"

#include "../../font/FontAwesome.h"

#include "../../SessionManager.hpp"

#include "../../AppState.hpp"
#include "../../anim/Animatable.hpp"

void WindowInspector::Level_Animation() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    this->DrawPreview(globalAnimatable);

    ImGui::SameLine();

    uint16_t animationIndex = globalAnimatable->getCurrentAnimationIndex();
    auto query = sessionManager.getCurrentSession()->getAnimationNames().find(animationIndex);

    const char* animationName = 
        query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
            query->second.c_str() : nullptr;

    ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

        ImGui::Text("Animation no. %u:", animationIndex + 1);

        ImGui::PushFont(appState.fontLarge);
        ImGui::TextWrapped("%s", animationName ? animationName : "(no macro defined)");
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();


    RvlCellAnim::Animation* animation = globalAnimatable->getCurrentAnimation();
    static char newMacroName[256]{ "" };

    ImGui::SeparatorText((char*)ICON_FA_PENCIL " Macro name");
    if (ImGui::Button("Edit macro name..")) {
        std::string copyName(animationName ? animationName : "");
        if (copyName.size() > 255)
            copyName.resize(255); // Truncate

        strcpy(newMacroName, copyName.c_str());
        ImGui::OpenPopup("###EditMacroNamePopup");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
    if (ImGui::BeginPopupModal("Edit macro name###EditMacroNamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Edit macro name for animation no. %u:", animationIndex);
        ImGui::InputText("##MacroNameInput", newMacroName, 256);

        ImGui::Dummy({ 0, 15 });
        ImGui::Separator();
        ImGui::Dummy({ 0, 5 });

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            changed = true;

            // Replace spaces with underscores
            {
                char* character = newMacroName;

                while (*character != '\0') {
                    if (*character == ' ')
                        *character = '_';
                    character++;
                }
            }

            query->second = newMacroName;
            ImGui::CloseCurrentPopup();
        } ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}