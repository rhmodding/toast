#include "WindowHybridList.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <sstream>
#include <string>

#include <cmath>

#include "../AppState.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../App/Popups.hpp"

#include "../command/CommandDeleteArrangement.hpp"
#include "../command/CommandInsertArrangement.hpp"
#include "../command/CommandModifyArrangement.hpp"
#include "../command/CommandModifyAnimation.hpp"
#include "../command/CommandDeleteAnimation.hpp"

void WindowHybridList::FlashWindow() {
    this->flashWindow = true;
    this->flashTimer = static_cast<float>(ImGui::GetTime());
}

void WindowHybridList::ResetFlash() {
    this->flashWindow = false;
    this->flashTrigger = false;
}

#define WINDOW_FLASH_TIME (.3f) // seconds

void WindowHybridList::Update() {
    GET_APP_STATE;

    static bool lastArrangementMode { appState.getArrangementMode() };
    if (lastArrangementMode != appState.getArrangementMode()) {
        this->flashTrigger = true;
        this->FlashWindow();
    }

    lastArrangementMode = appState.getArrangementMode();

    if (
        this->flashWindow &&
        (static_cast<float>(ImGui::GetTime()) - this->flashTimer > WINDOW_FLASH_TIME)
    )
        ResetFlash();

    if (this->flashWindow) {
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);

        float factor = (this->flashTimer - static_cast<float>(ImGui::GetTime())) + WINDOW_FLASH_TIME;
        if (factor > 1.f)
            factor = 1.f;

        color.x = std::lerp(color.x, 1.f, factor);
        color.y = std::lerp(color.y, 1.f, factor);
        color.z = std::lerp(color.z, 1.f, factor);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
    }

    const char* windowLabel = appState.getArrangementMode() ?
        "Arrangements" "###HybridList" :
        "Animations"   "###HybridList";
    
    ImGui::Begin(windowLabel);

    ImGui::BeginChild("List", { 0.f, 0.f }, ImGuiChildFlags_Border);
    {
        static RvlCellAnim::Arrangement copyArrangement;
        static bool allowPasteArrangement { false };

        static RvlCellAnim::Animation copyAnimation;
        static bool allowPasteAnimation { false };

        std::shared_ptr<BaseCommand> command;

        GET_SESSION_MANAGER;
        GET_ANIMATABLE;

        if (!appState.getArrangementMode())
            for (unsigned n = 0; n < globalAnimatable.cellanim->animations.size(); n++) {
                std::ostringstream fmtStream;

                const char* animName = globalAnimatable.cellanim->animations[n].name.c_str();
                if (animName == nullptr)
                    animName = "(no macro defined)";

                fmtStream << std::to_string(n+1) << ". " << animName;

                if (ImGui::Selectable(fmtStream.str().c_str(), appState.selectedAnimation == n, ImGuiSelectableFlags_SelectOnNav)) {
                    appState.selectedAnimation = n;

                    globalAnimatable.setAnimationFromIndex(appState.selectedAnimation);

                    PlayerManager::getInstance().setCurrentKeyIndex(0);

                    appState.correctSelectedParts();
                }

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::Text("Animation no. %u", n+1);
                    ImGui::Text("\"%s\"", animName);

                    ImGui::Separator();

                    if (ImGui::Selectable("Paste animation..", allowPasteAnimation)) {
                        command = std::make_shared<CommandModifyAnimation>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            n,
                            copyAnimation
                        );
                    }
                    if (ImGui::BeginMenu("Paste animation (special)..", allowPasteAnimation)) {
                        if (ImGui::MenuItem("..key timing")) {
                            auto newAnimation = globalAnimatable.cellanim->animations[n];

                            for (
                                unsigned i = 0;
                                i < newAnimation.keys.size() && i < copyAnimation.keys.size();
                                i++
                            ) {
                                newAnimation.keys[i].holdFrames = copyAnimation.keys[i].holdFrames;
                            }

                            command = std::make_shared<CommandModifyAnimation>(
                                sessionManager.getCurrentSession()->currentCellanim,
                                n,
                                newAnimation
                            );
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Copy animation")) {
                        copyAnimation = globalAnimatable.cellanim->animations[n];
                        allowPasteAnimation = true;
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Delete animation")) {
                        Popups::_deleteAnimationIdx = n;
                        appState.OpenGlobalPopup("###DeleteAnimation");
                    }

                    ImGui::EndPopup();
                }
            }
        else
            for (unsigned n = 0; n < globalAnimatable.cellanim->arrangements.size(); n++) {
                char buffer[48];
                snprintf(buffer, sizeof(buffer), "Arrangement no. %d", n+1);

                if (ImGui::Selectable(buffer, appState.controlKey.arrangementIndex == n, ImGuiSelectableFlags_SelectOnNav)) {
                    appState.controlKey.arrangementIndex = n;

                    appState.correctSelectedParts();
                }

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextUnformatted(buffer);
                    ImGui::Separator();

                    if (ImGui::Selectable("Insert new arrangement above")) {
                        command = std::make_shared<CommandInsertArrangement>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            n+1,
                            RvlCellAnim::Arrangement()
                        );

                        appState.controlKey.arrangementIndex = n + 1;
                    }
                    if (ImGui::Selectable("Insert new arrangement below")) {
                        command = std::make_shared<CommandInsertArrangement>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            n,
                            RvlCellAnim::Arrangement()
                        );

                        appState.controlKey.arrangementIndex = n;
                    }

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Paste arrangement..", allowPasteArrangement)) {
                        if (ImGui::MenuItem("..above")) {
                            command = std::make_shared<CommandInsertArrangement>(
                                sessionManager.getCurrentSession()->currentCellanim,
                                n+1,
                                copyArrangement
                            );

                            appState.controlKey.arrangementIndex = n + 1;
                        };
                        if (ImGui::MenuItem("..below")) {
                            command = std::make_shared<CommandInsertArrangement>(
                                sessionManager.getCurrentSession()->currentCellanim,
                                n,
                                copyArrangement
                            );

                            appState.controlKey.arrangementIndex = n;
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("..here (replace)")) {
                            command = std::make_shared<CommandModifyArrangement>(
                                sessionManager.getCurrentSession()->currentCellanim,
                                n,
                                copyArrangement
                            );
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::Selectable("Copy arrangement")) {
                        copyArrangement = globalAnimatable.cellanim->arrangements.at(n);
                        allowPasteArrangement = true;
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Delete arrangement")) {
                        command = std::make_shared<CommandDeleteArrangement>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            n
                        );
                    }

                    ImGui::EndPopup();
                }
            }

        if (command)
            sessionManager.getCurrentSession()->executeCommand(command);
    }
    ImGui::EndChild();

    ImGui::End();

    if (this->flashWindow)
        ImGui::PopStyleColor();
}
