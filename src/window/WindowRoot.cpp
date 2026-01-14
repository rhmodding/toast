#include "WindowRoot.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/AsyncTaskManager.hpp"
#include "manager/PromptPopupManager.hpp"

#include "App/PopupHandler.hpp"
#include "App/popups/AllPopups.hpp"

#include "App/Actions.hpp"

#include "App/Shortcuts.hpp"

#include "task/AsyncTaskPushSession.hpp"
#include "task/AsyncTaskExportSession.hpp"

#include "command/CommandSwitchCellanim.hpp"
#include "command/CommandMoveAnimationKey.hpp"
#include "command/CommandInsertAnimationKey.hpp"
#include "command/CommandDeleteAnimationKey.hpp"
#include "command/CommandModifyAnimationKey.hpp"
#include "command/CommandDeleteArrangement.hpp"
#include "command/CommandModifyArrangementPart.hpp"
#include "command/CommandDeleteArrangementPart.hpp"

#include "font/FontAwesome.h"

#include "Toast.hpp"

// TODO: include ordering
#include "WindowCanvas.hpp"
#include "WindowHybridList.hpp"
#include "WindowInspector.hpp"
#include "WindowTimeline.hpp"
#include "WindowSpritesheet.hpp"
#include "WindowConfig.hpp"
#include "WindowAbout.hpp"
#include "WindowImGuiDemo.hpp"

#define WINDOW_TITLE "toast"

template <typename T>
void WindowRoot::SubWindow<T>::update() {
    if (mIsShy) {
        return;
    }
    if (
        mMyOptions.showOnlyWithSession &&
        !SessionManager::getInstance().anySessionOpened()
    ) {
        return;
    }

#ifndef NDEBUG 
    if (UNLIKELY(mInst == nullptr)) {
        throw std::runtime_error(
            std::string("SubWindow<") + CxxDemangleUtil::Demangle<T>() +
            ">::update: window instance is NULL .."
        );
    }
#endif

    mInst->update();
}

WindowRoot::WindowRoot() {
    registerWindow<WindowAbout>(SubWindowOptions {
        .name = (const char *)ICON_FA_SHARE_FROM_SQUARE " About " WINDOW_TITLE,
        .showOnlyWithSession = false,
        .showInAppMenu = true,
    });
    registerWindow<WindowCanvas>(SubWindowOptions {
        .name = "Canvas",
        .showOnlyWithSession = true,
        .showInAppMenu = false,
    });
    registerWindow<WindowConfig>(SubWindowOptions {
        .name = (const char*)ICON_FA_WRENCH " Config",
        .showOnlyWithSession = false,
        .showInAppMenu = true,
    });
    registerWindow<WindowHybridList>(SubWindowOptions {
        .name = "Animation / arrangement list",
        .showOnlyWithSession = true,
        .showInAppMenu = false,
    });
    registerWindow<WindowImGuiDemo>(SubWindowOptions {
        .name = "Dear ImGui demo",
        .showOnlyWithSession = false,
        .showInAppMenu = false,
    });
    registerWindow<WindowInspector>(SubWindowOptions {
        .name = "Inspector",
        .showOnlyWithSession = true,
        .showInAppMenu = false,
    });
    registerWindow<WindowSpritesheet>(SubWindowOptions {
        .name = "Spritesheet",
        .showOnlyWithSession = true,
        .showInAppMenu = false,
    });
    registerWindow<WindowTimeline>(SubWindowOptions {
        .name = "Timeline",
        .showOnlyWithSession = true,
        .showInAppMenu = false,
    });
}

WindowRoot::~WindowRoot() {
    auto *window = mWindowHead;
    while (window != nullptr) {
        auto *next = window->getNext();
        delete window;
        window = next;
    }

    mWindowHead = nullptr;
    mWindowTail = nullptr;
}

void WindowRoot::update() {
    constexpr ImGuiDockNodeFlags dockspaceFlags =
        ImGuiDockNodeFlags_PassthruCentralNode;
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f, 0.f, 0.f, 0.f });

    ImGui::Begin(WINDOW_TITLE "###HeadWindow", nullptr, windowFlags);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGui::DockSpace(ImGui::GetID("HeadDockspace"), { -1.f, -1.f }, dockspaceFlags);
    }

    Shortcuts::process();

    doMenubar();

    for (auto *window = mWindowHead; window != nullptr; window = window->getNext()) {
        window->update();
    }

    if (SessionManager::getInstance().anySessionOpened()) {
        PlayerManager::getInstance().update();
    }

    PromptPopupManager::getInstance().update();

    Popups::update();

    ImGui::End();
}

template <typename T>
void WindowRoot::registerWindow(SubWindowOptions options) {
    static_assert(std::is_base_of<BaseWindow, T>::value, "T must be derived from BaseWindow");

    ISubWindow *window = static_cast<ISubWindow *>(new SubWindow<T>(std::move(options)));
    if (mWindowHead == nullptr) {
        mWindowHead = static_cast<SubWindow<> *>(window);
    }
    else {
        mWindowTail->setNext(window);
    }
    mWindowTail = static_cast<SubWindow<> *>(window);
}

void WindowRoot::doMenubar() {
    // const AppState& appState = AppState::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();
    ConfigManager& configManager = ConfigManager::getInstance();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(WINDOW_TITLE)) {
            for (auto *window = mWindowHead; window != nullptr; window = window->getNext()) {
                if (window->getOptions().showInAppMenu) {
                    window->openOption();
                    ImGui::Separator();
                }
            }

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Exit, ICON_FA_DOOR_OPEN).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Exit).c_str()
            )) {
                Toast::getInstance()->requestExit();
            }

            ImGui::EndMenu();
        }

        CellAnim::CellAnimType cellanimType { CellAnim::CELLANIM_TYPE_INVALID };
            if (sessionManager.anySessionOpened())
                cellanimType = sessionManager.getCurrentSession()->type;

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Open, ICON_FA_FILE_IMPORT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Open).c_str(),
                nullptr
            )) {
                Actions::CreateSessionPromptPath();
            }

            const auto& recentlyOpened = configManager.getConfig().recentlyOpened;

            if (ImGui::BeginMenu(
                (const char*)ICON_FA_FILE_IMPORT " Open recent",
                !recentlyOpened.empty()
            )) {
                for (size_t i = 0; i < Config::MAX_RECENTLY_OPENED; i++) {
                    const int j = recentlyOpened.size() > i ? recentlyOpened.size() - 1 - i : -1;
                    const bool none = (j < 0);

                    char label[128];
                    std::snprintf(label, sizeof(label), "%zu. %s", i + 1, none ? "-" : recentlyOpened[j].c_str());

                    ImGui::BeginDisabled(none);

                    if (ImGui::MenuItem(label) && !none) {
                        AsyncTaskManager::getInstance().startTask<AsyncTaskPushSession>(
                            recentlyOpened[j]
                        );
                    }

                    ImGui::EndDisabled();
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                (const char*)ICON_FA_FILE_IMPORT " Open containing folder...",
                nullptr, false, sessionManager.anySessionOpened()
            ))
                Actions::OpenSessionSourceFolder();

            ImGui::Separator();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Save, ICON_FA_FILE_EXPORT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Save).c_str(),
                false,
                sessionManager.anySessionOpened()
            ))
                Actions::ExportSession();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::SaveAs, ICON_FA_FILE_EXPORT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::SaveAs).c_str(),
                false,
                sessionManager.anySessionOpened()
            ))
                Actions::ExportSessionPromptPath();

            ImGui::Separator();

            const char* convertOptionTitle = (const char*)ICON_FA_FILE_EXPORT " Save as other...";
            if (cellanimType == CellAnim::CELLANIM_TYPE_RVL)
                convertOptionTitle = (const char*)ICON_FA_FILE_EXPORT " Save as Megamix (CTR) cellanim...";
            else if (cellanimType == CellAnim::CELLANIM_TYPE_CTR)
                convertOptionTitle = (const char*)ICON_FA_FILE_EXPORT " Save as Fever (RVL) cellanim...";

            if (ImGui::MenuItem(
                convertOptionTitle, nullptr, false, sessionManager.anySessionOpened()
            ))
                Actions::ExportSessionAsOther();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Undo, ICON_FA_ARROW_ROTATE_LEFT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Undo).c_str(),
                false,
                sessionManager.anySessionOpened() &&
                sessionManager.getCurrentSession()->canUndo()
            ))
                sessionManager.getCurrentSession()->undo();

            if (ImGui::MenuItem(
                Shortcuts::getDisplayName(Shortcuts::ShortcutAction::Redo, ICON_FA_ARROW_ROTATE_RIGHT).c_str(),
                Shortcuts::getDisplayChord(Shortcuts::ShortcutAction::Redo).c_str(),
                false,
                sessionManager.anySessionOpened() &&
                sessionManager.getCurrentSession()->canRedo()
            ))
                sessionManager.getCurrentSession()->redo();

            ImGui::EndMenu();
        }

        const bool sessionAvaliable = sessionManager.anySessionOpened();

        if (ImGui::BeginMenu("Cellanim", sessionAvaliable)) {
            if (ImGui::BeginMenu("Select")) {
                auto* currentSession = sessionManager.getCurrentSession();

                for (size_t i = 0; i < currentSession->cellanims.size(); i++) {
                    std::ostringstream fmtStream;
                    fmtStream << std::to_string(i+1) << ". " << currentSession->cellanims[i].object->getName();

                    if (ImGui::MenuItem(
                        fmtStream.str().c_str(), nullptr,
                        currentSession->getCurrentCellAnimIndex() == i
                    )) {
                        if (currentSession->getCurrentCellAnimIndex() != i) {
                            currentSession->addCommand(
                            std::make_shared<CommandSwitchCellanim>(
                                sessionManager.getCurrentSessionIndex(), i
                            ));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Optimize .."))
                Popups::MOptimizeGlobal::getInstance().open();

            ImGui::Separator();

            if (ImGui::MenuItem("Transform .."))
                Popups::MTransformCellanim::getInstance().open();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Spritesheets", sessionAvaliable)) {
            if (ImGui::MenuItem("Open spritesheet manager .."))
                Popups::SpritesheetManager::getInstance().open();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Animation", sessionAvaliable)) {
            const unsigned animIndex = playerManager.getAnimationIndex();

            ImGui::Text("Selected animation (no. %u)", animIndex + 1);

            ImGui::Separator();

            if (ImGui::MenuItem("Edit name ..")) {
                Popups::EditAnimationName::getInstance().setAnimIndex(animIndex);
                Popups::EditAnimationName::getInstance().open();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Swap index ..")) {
                Popups::SwapAnimation::getInstance().setAnimationIndex(animIndex);
                Popups::SwapAnimation::getInstance().open();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Transform .."))
                Popups::MTransformAnimation::getInstance().open();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Key",
            sessionAvaliable &&
            !sessionManager.getInstance().getCurrentSession()->arrangementMode &&
            !playerManager.getPlaying()
        )) {
            const unsigned keyIndex = playerManager.getKeyIndex();
            const auto& key = playerManager.getKey();

            ImGui::Text(
                "Selected key (no. %u)",
                keyIndex + 1
            );

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Tween ..",
                nullptr, nullptr,
                keyIndex + 1 < playerManager.getKeyCount()
            ))
                Popups::MInterpolateKeys::getInstance().open();

            ImGui::Separator();

            if (ImGui::BeginMenu("Move selected key ..")) {
                ImGui::BeginDisabled(keyIndex == (playerManager.getKeyCount() - 1));
                if (ImGui::MenuItem(".. up")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex,
                        false,
                        false
                    ));

                    playerManager.setKeyIndex(keyIndex + 1);
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(keyIndex == 0);
                if (ImGui::MenuItem(".. down")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex,
                        true,
                        false
                    ));

                    playerManager.setKeyIndex(keyIndex - 1);
                }
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Duplicate selected key ..")) {
                if (ImGui::MenuItem(".. after")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex + 1,
                        key
                    ));

                    playerManager.setKeyIndex(keyIndex + 1);
                }

                if (ImGui::MenuItem(".. before")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        keyIndex,
                        key
                    ));

                    playerManager.setKeyIndex(keyIndex);
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete selected key")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteAnimationKey>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getAnimationIndex(),
                    playerManager.getKeyIndex()
                ));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Arrangement",
            sessionAvaliable &&
            !playerManager.getPlaying()
        )) {
            const unsigned arrangementIndex = playerManager.getArrangementIndex();

            ImGui::Text(
                "Current arrangement (no. %u)",
                arrangementIndex + 1
            );

            ImGui::Separator();

            if (ImGui::MenuItem("Make arrangement unique (duplicate)")) {
                auto& cellAnim = *sessionManager.getCurrentSession()->getCurrentCellAnim().object;

                auto& key = playerManager.getKey();

                unsigned index = cellAnim.duplicateArrangement(key.arrangementIndex);

                if (!sessionManager.getCurrentSession()->arrangementMode) {
                    auto newKey = key;
                    newKey.arrangementIndex = index;

                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandModifyAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        playerManager.getKeyIndex(),
                        newKey
                    ));
                }
                else
                    key.arrangementIndex = index;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Transform as whole .."))
                Popups::MTransformArrangement::getInstance().open();

            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteArrangement>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    arrangementIndex
                ));
            }

            ImGui::EndMenu();
        }

        bool singlePartSelected = false;
        if (sessionAvaliable) {
            const auto& selectionState = sessionManager.getCurrentSession()->getPartSelectState();
            singlePartSelected = selectionState.singleSelected();
        }

        if (ImGui::BeginMenu("Part",
            sessionAvaliable &&
            singlePartSelected &&
            !playerManager.getPlaying()
        )) {
            const auto& selectionState = sessionManager.getCurrentSession()->getPartSelectState();

            auto& part = playerManager.getArrangement().parts.at(selectionState.mSelected[0].index);

            ImGui::Text(
                "Selected part (no. %u)",
                selectionState.mSelected[0].index + 1
            );

            ImGui::Separator();

            ImGui::MenuItem("Visible", nullptr, &part.editorVisible);
            ImGui::MenuItem("Locked", nullptr, &part.editorLocked);

            ImGui::Separator();

            if (ImGui::BeginMenu("Cell")) {
                if (ImGui::MenuItem("Pad (Expand/Contract) .."))
                    Popups::MPadRegion::getInstance().open();

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Set editor name ..")) {
                PromptPopupManager::getInstance().queue(
                    PromptPopupManager::createPrompt(
                        "Edit part name",
                        "Please enter the part's new name:"
                    )
                    .withInput(part.editorName)
                    .withResponses(
                        PromptPopup::RESPONSE_OK | PromptPopup::RESPONSE_CANCEL,
                        PromptPopup::RESPONSE_OK
                    )
                    .withCallback(
                        [
                            arrangementIndex = playerManager.getArrangementIndex(),
                            partIndex = selectionState.mSelected[0].index
                        ]
                        (auto res, const std::string* newName) {
                            auto session = SessionManager::getInstance().getCurrentSession();
                            auto& anim = session->getCurrentCellAnim();
                            auto newPart = anim.object->getArrangement(arrangementIndex).parts.at(partIndex);

                            newPart.editorName = *newName;

                            session->addCommand(std::make_shared<CommandModifyArrangementPart>(
                                session->getCurrentCellAnimIndex(),
                                arrangementIndex, partIndex, newPart)
                            );
                        }
                    )
                );
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteArrangementPart>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getArrangementIndex(),
                    selectionState.mSelected[0].index
                ));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            const bool shiftHeld = ImGui::GetIO().KeyShift;

            // TODO: In which other menus should shift inhibit closing on click?
            // XXX: when this is removed, remove the respective PopItemFlag below
            if (shiftHeld) {
                ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
            }

            for (auto *window = mWindowHead; window != nullptr; window = window->getNext()) {
                if (!window->getOptions().showInAppMenu) {
                    window->toggleShyOption();
                }
            }

            if (shiftHeld) {
                ImGui::PopItemFlag();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMenuBar()) {
        constexpr ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll |
            ImGuiTabBarFlags_AutoSelectNewTabs;

        SessionManager& sessionManager = SessionManager::getInstance();

        if (ImGui::BeginTabBar("SessionTabs", tabBarFlags)) {
            for (int n = 0; n < static_cast<int>(sessionManager.getSessionCount()); n++) {
                ImGui::PushID(n);

                auto& session = sessionManager.getSession(n);

                bool sessionOpen { true };
                bool tabVisible = ImGui::BeginTabItem(
                    session.resourcePath.c_str(),
                    &sessionOpen,
                    session.modified ?
                        ImGuiTabItemFlags_UnsavedDocument :
                        ImGuiTabItemFlags_None
                );

                if (tabVisible)
                    ImGui::EndTabItem();

                if (ImGui::IsItemClicked() && sessionManager.getCurrentSessionIndex() != n) {
                    sessionManager.setCurrentSessionIndex(n);
                }

                if (tabVisible && ImGui::BeginPopupContextItem()) {
                    ImGui::TextUnformatted("Select a Cellanim:");
                    ImGui::Separator();
                    for (size_t i = 0; i < session.cellanims.size(); i++) {
                        std::ostringstream fmtStream;
                        fmtStream << std::to_string(i+1) << ". " << session.cellanims[i].object->getName();

                        if (ImGui::MenuItem(
                            fmtStream.str().c_str(), nullptr,
                            session.getCurrentCellAnimIndex() == i
                        )) {
                            ImGui::CloseCurrentPopup();

                            if (session.getCurrentCellAnimIndex() != i) {
                                sessionManager.setCurrentSessionIndex(n);

                                // TODO: bug here probably
                                session.addCommand(
                                std::make_shared<CommandSwitchCellanim>(
                                    n, i
                                ));
                            }
                        }
                    }

                    ImGui::EndPopup();
                }

                ImGui::SetItemTooltip(
                    "Path: %s\nCellanim: %s\n\nRight-click to select the cellanim.",
                    session.resourcePath.c_str(),
                    session.getCurrentCellAnim().object->getName().c_str()
                );

                if (!sessionOpen) {
                    if (session.modified) {
                        PromptPopupManager::getInstance().queue(
                            PromptPopupManager::createPrompt(
                                "Hang on!",
                                "There are still unsaved changes in this session; are you sure\n"
                                "you want to close it?"
                            )
                            .withResponses(
                                PromptPopup::RESPONSE_CANCEL | PromptPopup::RESPONSE_YES,
                                PromptPopup::RESPONSE_CANCEL
                            )
                            .withCallback([n](auto res, const std::string*) {
                                if (res == PromptPopup::RESPONSE_YES) {
                                    SessionManager::getInstance().removeSession(n);
                                }
                            })
                        );
                    }
                    else {
                        sessionManager.removeSession(n);
                    }
                }

                ImGui::PopID();
            }
            ImGui::EndTabBar();
        }

        ImGui::EndMenuBar();
    }
}

