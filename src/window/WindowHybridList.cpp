#include "WindowHybridList.hpp"

#include <cstdint>

#include <sstream>

#include <string>
#include <string_view>

#include <set>

#include <cmath>

#include <imgui.h>
#include <imgui_internal.h>

#include "manager/AppState.hpp"
#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/PromptPopupManager.hpp"

#include "command/CommandDeleteArrangement.hpp"
#include "command/CommandInsertArrangement.hpp"
#include "command/CommandModifyArrangement.hpp"
#include "command/CommandModifyArrangements.hpp"
#include "command/CommandInsertAnimation.hpp"
#include "command/CommandModifyAnimation.hpp"
#include "command/CommandModifyAnimationName.hpp"
#include "command/CommandDeleteAnimation.hpp"

#include "command/CompositeCommand.hpp"

constexpr float WINDOW_FLASH_TIME = .3f; // seconds

void WindowHybridList::flashWindow() {
    mFlashWindow = true;
    mFlashTimer = static_cast<float>(ImGui::GetTime());
}
void WindowHybridList::resetFlash() {
    mFlashWindow = false;
    mFlashTrigger = false;
}

static CellAnim::Animation createNewAnimation() {
    constexpr std::string_view namePrefix = "CellAnim";

    SessionManager& sessionManager = SessionManager::getInstance();
    const auto& animations = sessionManager.getCurrentSession()
        ->getCurrentCellAnim().object->getAnimations();

    std::set<unsigned> usedNumbers;
    for (const auto& animation : animations) {
        if (animation.name.compare(0, namePrefix.size(), namePrefix.data()) == 0) {
            std::string suffix = animation.name.substr(namePrefix.size());
            bool isNumber = !suffix.empty() && std::all_of(suffix.begin(), suffix.end(), isdigit);
            if (isNumber)
                usedNumbers.insert(std::stoi(suffix));
        }
    }

    unsigned nextNumber = 0;
    while (usedNumbers.count(nextNumber))
        ++nextNumber;

    CellAnim::Animation newAnimation {
        .keys = {
            CellAnim::AnimationKey {}
        },
        .name = std::string(namePrefix) + std::to_string(nextNumber)
    };
    return newAnimation;
}

void WindowHybridList::update() {
    SessionManager& sessionManager = SessionManager::getInstance();

    int currentSessionIdx = sessionManager.getCurrentSessionIndex();
    Session* currentSession = sessionManager.getCurrentSession();

    static bool lastArrangementMode { currentSession->arrangementMode };
    if (lastArrangementMode != currentSession->arrangementMode) {
        lastArrangementMode = currentSession->arrangementMode;

        mFlashTrigger = true;
        flashWindow();
    }

    if (
        mFlashWindow &&
        (static_cast<float>(ImGui::GetTime()) - mFlashTimer > WINDOW_FLASH_TIME)
    ) {
        resetFlash();
    }

    if (mFlashWindow) {
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);

        float factor = (mFlashTimer - static_cast<float>(ImGui::GetTime())) + WINDOW_FLASH_TIME;
        if (factor > 1.f)
            factor = 1.f;

        color.x = std::lerp(color.x, 1.f, factor);
        color.y = std::lerp(color.y, 1.f, factor);
        color.z = std::lerp(color.z, 1.f, factor);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
    }

    const char* windowLabel = currentSession->arrangementMode ?
        "Arrangements" "###HybridList" :
        "Animations"   "###HybridList";

    ImGui::Begin(windowLabel);

    ImGui::BeginChild("List", { 0.f, 0.f }, ImGuiChildFlags_Borders);
    {
        static CellAnim::Arrangement copyArrangement;
        static bool allowPasteArrangement { false };

        static int copyAnimationSrcSession { -1 }; // Index of session where copied animation originates.
        static int copyAnimationSrcCellAnim { -1 }; // Index of cellanim where copied animation originates.
        static std::vector<CellAnim::Arrangement> copyAnimationArrangements;
        static CellAnim::Animation copyAnimation;
        static bool allowPasteAnimation { false };

        std::shared_ptr<BaseCommand> command;
        int newAnimationSelect = -1;
        int newAnimKeySet = -1;
        int newArrangementSelect = -1;

        PlayerManager& playerManager = PlayerManager::getInstance();

        const auto& config = ConfigManager::getInstance().getConfig();

        const auto& animations = currentSession->getCurrentCellAnim().object->getAnimations();
        const auto& arrangements = currentSession->getCurrentCellAnim().object->getArrangements();

        if (!currentSession->arrangementMode) {
            for (unsigned n = 0; n < animations.size(); n++) {
                std::ostringstream fmtStream;

                const char* animName = animations[n].name.c_str();
                if (animName[0] == '\0')
                    animName = "(no name set)";

                fmtStream << std::to_string(n+1) << ". " << animName;

                if (ImGui::Selectable(
                    fmtStream.str().c_str(),
                    playerManager.getAnimationIndex() == n,
                    ImGuiSelectableFlags_SelectOnNav
                )) {
                    newAnimationSelect = n;
                    newAnimKeySet = 0;
                }

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::Text("Animation no. %u", n+1);
                    ImGui::Text("\"%s\"", animName);

                    ImGui::Separator();

                    if (ImGui::Selectable("Clear keys")) {
                        CellAnim::Animation newAnimation;
                        newAnimation.keys.emplace_back(); // Add one defaulted key.
                        newAnimation.name = animations[n].name;
                        newAnimation.isInterpolated = animations[n].isInterpolated;

                        command = std::make_shared<CommandModifyAnimation>(
                            currentSession->getCurrentCellAnimIndex(),
                            n,
                            newAnimation
                        );
                    }

                    ImGui::Separator();

                    if (config.allowNewAnimCreate) {
                        if (ImGui::Selectable("Insert new animation above")) {
                            command = std::make_shared<CommandInsertAnimation>(
                                currentSession->getCurrentCellAnimIndex(),
                                n+1,
                                createNewAnimation()
                            );
                            newAnimationSelect = n+1;
                        }
                        if (ImGui::Selectable("Insert new animation below")) {
                            command = std::make_shared<CommandInsertAnimation>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                createNewAnimation()
                            );
                            newAnimationSelect = n;
                        }

                        ImGui::Separator();
                    }

                    if (ImGui::Selectable("Paste animation..", allowPasteAnimation)) {
                        CellAnim::Animation newAnimation = copyAnimation;
                        newAnimation.name = animations[n].name;

                        if (
                            currentSessionIdx != copyAnimationSrcSession ||
                            currentSession->getCurrentCellAnimIndex() != copyAnimationSrcCellAnim
                        ) {
                            auto composite = std::make_shared<CompositeCommand>();
                            command = composite;

                            auto& cellAnim = *currentSession->getCurrentCellAnim().object;

                            std::vector<CellAnim::Arrangement> newArrangements = cellAnim.getArrangements();

                            auto baseIndex = newArrangements.size(); // BEFORE insertion
                            newArrangements.insert(newArrangements.end(), copyAnimationArrangements.begin(), copyAnimationArrangements.end());

                            for (size_t i = 0; i < newAnimation.keys.size(); ++i) {
                                newAnimation.keys[i].arrangementIndex = baseIndex + i;
                            }

                            composite->addCommand(std::make_shared<CommandModifyArrangements>(
                                currentSession->getCurrentCellAnimIndex(),
                                newArrangements
                            ));
                            composite->addCommand(std::make_shared<CommandModifyAnimation>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                newAnimation
                            ));
                        }
                        else {
                            command = std::make_shared<CommandModifyAnimation>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                newAnimation
                            );
                        }
                    }
                    if (ImGui::BeginMenu("Paste animation (special)..", allowPasteAnimation)) {
                        if (ImGui::MenuItem("..key timing")) {
                            // Copy
                            auto newAnimation = animations[n];

                            for (
                                size_t i = 0;
                                i < newAnimation.keys.size() && i < copyAnimation.keys.size();
                                i++
                            ) {
                                newAnimation.keys[i].holdFrames = copyAnimation.keys[i].holdFrames;
                            }

                            command = std::make_shared<CommandModifyAnimation>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                newAnimation
                            );
                        }

                        if (ImGui::MenuItem("..name")) {
                            command = std::make_shared<CommandModifyAnimationName>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                copyAnimation.name
                            );
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Copy animation")) {
                        copyAnimation = animations[n];
                        copyAnimationArrangements.resize(copyAnimation.keys.size());
                        for (size_t i = 0; i < copyAnimation.keys.size(); i++) {
                            copyAnimationArrangements[i] = arrangements.at(copyAnimation.keys[i].arrangementIndex);
                        }

                        copyAnimationSrcSession = currentSessionIdx;
                        copyAnimationSrcCellAnim = currentSession->getCurrentCellAnimIndex();

                        allowPasteAnimation = true;
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Delete animation")) {
                        std::string promptMsg =
                            "Are you sure you want to delete animation no. " +
                            std::to_string(n) + "?";

                        if (n != (animations.size() - 1)) {
                            promptMsg += "\nThis change will shift other animations indices.";
                        }
                        else {
                            promptMsg += "\n"
                                "The game will likely crash if it attempts to load an animation\n"
                                "at this index.";
                        }

                        PromptPopupManager::getInstance().queue(
                            PromptPopupManager::createPrompt("Hang on!", promptMsg)
                            .withResponses(
                                PromptPopup::RESPONSE_YES | PromptPopup::RESPONSE_CANCEL,
                                PromptPopup::RESPONSE_CANCEL
                            )
                            .withCallback([n](auto res, const std::string*) {
                                if (res == PromptPopup::RESPONSE_YES) {
                                    SessionManager& sessionManager = SessionManager::getInstance();

                                    sessionManager.getCurrentSession()->addCommand(
                                    std::make_shared<CommandDeleteAnimation>(
                                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                        n
                                    ));
                                }
                            })
                        );
                    }

                    ImGui::EndPopup();
                }
            }
        }
        else {
            for (unsigned n = 0; n < arrangements.size(); n++) {
                char buffer[48];
                std::snprintf(buffer, sizeof(buffer), "Arrangement no. %d", n+1);

                if (ImGui::Selectable(buffer, playerManager.getArrangementModeIdx() == n, ImGuiSelectableFlags_SelectOnNav)) {
                    newArrangementSelect = n;
                }

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextUnformatted(buffer);
                    ImGui::Separator();

                    if (ImGui::Selectable("Insert new arrangement above")) {
                        command = std::make_shared<CommandInsertArrangement>(
                            currentSession->getCurrentCellAnimIndex(),
                            n+1,
                            CellAnim::Arrangement()
                        );
                        newArrangementSelect = n+1;
                    }
                    if (ImGui::Selectable("Insert new arrangement below")) {
                        command = std::make_shared<CommandInsertArrangement>(
                            currentSession->getCurrentCellAnimIndex(),
                            n,
                            CellAnim::Arrangement()
                        );
                        newArrangementSelect = n;
                    }

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Paste arrangement..", allowPasteArrangement)) {
                        if (ImGui::MenuItem("..above")) {
                            command = std::make_shared<CommandInsertArrangement>(
                                currentSession->getCurrentCellAnimIndex(),
                                n+1,
                                copyArrangement
                            );
                            newArrangementSelect = n+1;
                        }
                        if (ImGui::MenuItem("..below")) {
                            command = std::make_shared<CommandInsertArrangement>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                copyArrangement
                            );
                            newArrangementSelect = n;
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("..here (replace)")) {
                            command = std::make_shared<CommandModifyArrangement>(
                                currentSession->getCurrentCellAnimIndex(),
                                n,
                                copyArrangement
                            );
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::Selectable("Copy arrangement")) {
                        copyArrangement = arrangements[n];
                        allowPasteArrangement = true;
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Delete arrangement")) {
                        command = std::make_shared<CommandDeleteArrangement>(
                            currentSession->getCurrentCellAnimIndex(),
                            n
                        );
                    }

                    ImGui::EndPopup();
                }
            }
        }

        if (command)
            currentSession->addCommand(command);

        if (newAnimationSelect >= 0) {
            playerManager.setAnimationIndex(newAnimationSelect);
            playerManager.validateState();
        }
        if (newAnimKeySet >= 0) {
            playerManager.setKeyIndex(newAnimKeySet);
            playerManager.validateState();
        }
        if (newArrangementSelect >= 0) {
            playerManager.setArrangementModeIdx(newArrangementSelect);
            playerManager.validateState();
        }
    }
    ImGui::EndChild();

    ImGui::End();

    if (mFlashWindow)
        ImGui::PopStyleColor();
}
