#include "WindowHybridList.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <sstream>
#include <string>

#include <cmath>

#include "../AppState.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../command/CommandDeleteArrangement.hpp"
#include "../command/CommandInsertArrangement.hpp"
#include "../command/CommandModifyArrangement.hpp"

void WindowHybridList::FlashWindow() {
    this->flashWindow = true;
    this->flashTimer = static_cast<float>(ImGui::GetTime());
}

void WindowHybridList::ResetFlash() {
    this->flashWindow = false;
    this->flashTrigger = false;
}

ImVec4 adjustBrightness(const ImVec4& color, float factor) {
    ImVec4 adjustedColor;

    adjustedColor.x = std::lerp(color.x, 1.f, factor);
    adjustedColor.y = std::lerp(color.y, 1.f, factor);
    adjustedColor.z = std::lerp(color.z, 1.f, factor);
    adjustedColor.w = color.w;

    adjustedColor.x = std::min(std::max(adjustedColor.x, 0.f), 1.f);
    adjustedColor.y = std::min(std::max(adjustedColor.y, 0.f), 1.f);
    adjustedColor.z = std::min(std::max(adjustedColor.z, 0.f), 1.f);

    return adjustedColor;
}

#define WINDOW_FLASH_TIME .3f // seconds

void WindowHybridList::Update() {
    GET_APP_STATE;

    static bool lastArrangementMode{ appState.getArrangementMode() };
    if (lastArrangementMode != appState.getArrangementMode()) {
        this->flashTrigger = true;
        this->FlashWindow();
    }

    lastArrangementMode = appState.getArrangementMode();

    if (
        UNLIKELY(this->flashWindow) &&
        (static_cast<float>(ImGui::GetTime()) - this->flashTimer > WINDOW_FLASH_TIME)
    )
        ResetFlash();

    if (UNLIKELY(this->flashWindow)) {
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            adjustBrightness(
                color,
                (this->flashTimer - static_cast<float>(ImGui::GetTime())) + WINDOW_FLASH_TIME
            )
        );
    }

    ImGui::Begin(appState.getArrangementMode() ? "Arrangements###HybridList" : "Animations###HybridList", nullptr);

    ImGui::BeginChild("List", ImVec2(0, 0), ImGuiChildFlags_Border);
    {
        int deleteArrangement{ -1 };

        static RvlCellAnim::Arrangement copyArrangement;
        static bool allowPasteArrangement{ false };

        static RvlCellAnim::Animation copyAnimation;
        static bool allowPasteAnimation{ false };

        std::shared_ptr<BaseCommand> command;

        GET_SESSION_MANAGER;
        GET_ANIMATABLE;

        if (!appState.getArrangementMode())
            for (unsigned n = 0; n < globalAnimatable->cellanim->animations.size(); n++) {
                std::ostringstream fmtStream;

                auto query = sessionManager.getCurrentSession()->getAnimationNames().find(n);

                const char* animName =
                    query != sessionManager.getCurrentSession()->getAnimationNames().end() ?
                        query->second.c_str() :
                        "(no macro defined)";

                fmtStream << std::to_string(n+1) << ". " << animName;

                if (ImGui::Selectable(fmtStream.str().c_str(), appState.selectedAnimation == n, ImGuiSelectableFlags_SelectOnNav)) {
                    appState.selectedAnimation = n;

                    globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);

                    PlayerManager::getInstance().setCurrentKeyIndex(0);

                    appState.correctSelectedPart();
                }

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::Text("Animation no. %u", n+1);
                    ImGui::Text("\"%s\"", animName);

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Paste animation..", allowPasteAnimation)) {
                        ImGui::MenuItem("..above");
                        ImGui::MenuItem("..below");

                        ImGui::Separator();

                        ImGui::MenuItem("..here (replace)");

                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Paste animation (special)..", allowPasteAnimation)) {
                        ImGui::MenuItem("..key timing");

                        ImGui::EndMenu();
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Copy animation")) {
                        copyAnimation = globalAnimatable->cellanim->animations.at(n);
                        allowPasteAnimation = true;
                    }

                    ImGui::Separator();

                    ImGui::Selectable("Delete animation");

                    ImGui::EndPopup();
                }
            }
        else
            for (unsigned n = 0; n < globalAnimatable->cellanim->arrangements.size(); n++) {
                char buffer[32];
                sprintf(buffer, "Arrangement no. %d", n+1);

                RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

                if (ImGui::Selectable(buffer, appState.controlKey.arrangementIndex == n, ImGuiSelectableFlags_SelectOnNav)) {
                    appState.controlKey.arrangementIndex = n;

                    // Update arrangementPtr
                    arrangementPtr = globalAnimatable->getCurrentArrangement();

                    appState.correctSelectedPart();
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

                    if (ImGui::BeginMenu("Paste arrangement..")) {
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
                        copyArrangement = globalAnimatable->cellanim->arrangements.at(n);
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
