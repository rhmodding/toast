#include "WindowTimeline.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <limits>

#include <cstdint>

#include "../font/FontAwesome.h"

#include "../AppState.hpp"
#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../command/CommandMoveAnimationKey.hpp"
#include "../command/CommandDeleteAnimationKey.hpp"
#include "../command/CommandInsertAnimationKey.hpp"
#include "../command/CommandModifyAnimationKey.hpp"
#include "../command/CommandModifyAnimation.hpp"

static const uint32_t u32_one = 1;

static const uint8_t  u8_min  = 0;
static const uint8_t  u8_max  = 0xFFu;

void WindowTimeline::Update() {
    AppState& appState = AppState::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    const ImGuiIO& io = ImGui::GetIO(); 

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::Begin("Timeline");
    ImGui::PopStyleVar();

    ImGui::BeginDisabled(appState.getArrangementMode());

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);

    ImGui::BeginChild("TimelineToolbar", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::Dummy({ 0.f, .5f });

        ImGui::Dummy({ 2.f, 0.f });
        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 15.f, 0.f });

        if (ImGui::BeginTable("TimelineToolbarTable", 3, ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableNextRow();

            // All playback buttons
            ImGui::TableSetColumnIndex(0);

            char playPauseButtonLabel[24] { '\0' };
            const char* playPauseIcon = playerManager.playing ? (const char*)ICON_FA_PAUSE : (const char*)ICON_FA_PLAY;

            snprintf(
                playPauseButtonLabel, sizeof(playPauseButtonLabel),
                "%s##playPauseButton", playPauseIcon
            );

            const ImVec2 normalButtonSize { 32, 32 };
            const ImVec2 smallButtonSize  { 32 - 6, 32 - 6 };

            if (ImGui::Button(playPauseButtonLabel, normalButtonSize)) {
                if (
                    (playerManager.getKeyIndex() == playerManager.getKeyCount() - 1) &&
                    (playerManager.getHoldFramesLeft() == 0)
                )
                    playerManager.setKeyIndex(0);

                playerManager.ResetTimer();
                playerManager.setPlaying(!playerManager.playing);
            }
            ImGui::SameLine();

            ImGui::Dummy({ 2.f, 0.f });
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            if (ImGui::Button((const char*)ICON_FA_BACKWARD_FAST "##firstFrameButton", smallButtonSize)) {
                playerManager.setKeyIndex(0);
            } ImGui::SameLine();

            ImGui::SetItemTooltip("Go to first key");

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            if (ImGui::Button((const char*)ICON_FA_BACKWARD_STEP "##backFrameButton", smallButtonSize)) {
                if (playerManager.getKeyIndex() >= 1)
                    playerManager.setKeyIndex(playerManager.getKeyIndex() - 1);
            } ImGui::SameLine();

            ImGui::SetItemTooltip("Step back a key");

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            if (ImGui::Button((const char*)ICON_FA_STOP "##stopButton", smallButtonSize)) {
                playerManager.setPlaying(false);
                playerManager.setKeyIndex(0);
            } ImGui::SameLine();

            ImGui::SetItemTooltip("Stop playback and go to first key");

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            if (ImGui::Button((const char*)ICON_FA_FORWARD_STEP "##forwardFrameButton", smallButtonSize)) {
                if (playerManager.getKeyIndex() != playerManager.getKeyCount() - 1) {
                    playerManager.setKeyIndex(playerManager.getKeyIndex() + 1);
                }
            } ImGui::SameLine();

            ImGui::SetItemTooltip("Step forward a key");

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            if (ImGui::Button((const char*)ICON_FA_FORWARD_FAST "##lastFrameButton", smallButtonSize)) {
                playerManager.setKeyIndex(playerManager.getKeyCount() - 1);
            } ImGui::SameLine();

            ImGui::SetItemTooltip("Go to last key");

            ImGui::Dummy({ 2.f, 0.f });
            ImGui::SameLine();

            {
                const bool lLooping = playerManager.looping;
                if (lLooping)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                if (ImGui::Button((const char*)ICON_FA_ARROW_ROTATE_RIGHT "##loopButton", normalButtonSize))
                    playerManager.looping ^= true;

                ImGui::SetItemTooltip("Toggle looping");

                if (lLooping)
                    ImGui::PopStyleColor();
            }

            // Key No., Hold Frames Left & FPS
            ImGui::TableSetColumnIndex(1);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);

            unsigned keyNo = playerManager.getKeyIndex() + 1;

            ImGui::SetNextItemWidth(52.f);
            if (ImGui::InputScalar("Key No.", ImGuiDataType_U32, &keyNo, nullptr, nullptr, "%u")) {
                playerManager.setKeyIndex(std::clamp<unsigned>(keyNo - 1, 0, playerManager.getKeyCount() - 1));
            }

            ImGui::SameLine();

            int holdFramesLeft = playerManager.getHoldFramesLeft();

            ImGui::BeginDisabled(true);

            ImGui::SetNextItemWidth(52.f);
            ImGui::InputInt("Hold Frames Left", &holdFramesLeft, 0, 0, ImGuiInputTextFlags_ReadOnly);

            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::SetNextItemWidth(52.f);
            ImGui::InputScalar("FPS", ImGuiDataType_U32, &playerManager.frameRate, nullptr, nullptr, "%u");
            
            // Onion skin options
            ImGui::TableSetColumnIndex(2);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            if (ImGui::Button((const char*)ICON_FA_EYE " Onion Skin ..", { 0.f, 32.f - 6.f }))
                ImGui::OpenPopup("###OnionSkinOptions");

            if (ImGui::BeginPopup("###OnionSkinOptions")) {
                ImGui::Checkbox("Enabled", &appState.onionSkinState.enabled);

                ImGui::Separator();

                ImGui::InputScalar("Back count", ImGuiDataType_U32, &appState.onionSkinState.backCount, &u32_one);
                ImGui::InputScalar("Front count", ImGuiDataType_U32, &appState.onionSkinState.frontCount, &u32_one);

                ImGui::Separator();

                ImGui::DragScalar(
                    "Opacity",
                    ImGuiDataType_U8, &appState.onionSkinState.opacity,
                    1.f, &u8_min, &u8_max,
                    "%u/255",
                    ImGuiSliderFlags_AlwaysClamp
                );

                ImGui::Separator();

                ImGui::Checkbox("Draw behind", &appState.onionSkinState.drawUnder);

                ImGui::Checkbox("Roll over (loop)", &appState.onionSkinState.rollOver);

                ImGui::EndPopup();
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        ImGui::Dummy({ 0.f, .5f });
    }
    ImGui::EndChild();

    ImGui::BeginChild("TimelineKeys", { 0.f, 0.f }, 0, ImGuiWindowFlags_HorizontalScrollbar);
    {
        const ImVec2 buttonDimensions { 22.f, 30.f };
        const float holdFrameWidth { 22.f / 2 };

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 15.f, 0.f });

        if (ImGui::BeginTable(
            "TimelineFrameTable", 2,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_ScrollX
        )) {
            // Keys
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::Dummy({ 5.f, 0.f });
            ImGui::SameLine();
            ImGui::TextUnformatted("Keys");
            
            ImGui::TableSetColumnIndex(1);

            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 3.f });
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.f, 4.f });
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

                for (unsigned i = 0; i < playerManager.getKeyCount(); i++) {
                    ImGui::PushID(i);

                    bool popColor { false };
                    if (playerManager.getKeyIndex() == i) {
                        popColor = true;
                        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    }

                    char buffer[24];
                    snprintf(
                        buffer, sizeof(buffer),
                        "%u##KeyButton", i+1
                    );

                    if (ImGui::Button(buffer, buttonDimensions)) {
                        playerManager.setKeyIndex(i);
                    }

                    CellAnim::AnimationKey& key = playerManager.getAnimation().keys.at(i);

                    enum DeleteKeyMode {
                        DeleteKeyMode_None,

                        DeleteKeyMode_Current,

                        DeleteKeyMode_ToRight,
                        DeleteKeyMode_ToLeft
                    } deleteKeyMode { DeleteKeyMode_None };

                    if (ImGui::BeginPopupContextItem()) {
                        ImGui::Text((const char*)ICON_FA_KEY "  Options for key no. %u", i+1);
                        ImGui::TextUnformatted(
                            "Hold "
                        #if defined(__APPLE__)
                            "[Option]"
                        #else
                            "[Alt]"
                        #endif // defined(__APPLE__)
                            " for variations."
                        );

                        ImGui::Separator();

                        // Key splitting
                        {
                            bool splitPossible { false };
                            const CellAnim::Arrangement* arrangementA { nullptr };
                            const CellAnim::Arrangement* arrangementB { nullptr };

                            auto& arrangements =
                                sessionManager.getCurrentSession()
                                    ->getCurrentCellanim().object->arrangements;

                            if (i+1 < playerManager.getKeyCount()) {
                                arrangementA = &arrangements.at(key.arrangementIndex);
                                arrangementB = &arrangements.at(
                                    playerManager.getAnimation().keys.at(i+1).arrangementIndex
                                );

                                splitPossible = arrangementA->parts.size() == arrangementB->parts.size();
                            }

                            ImGui::BeginDisabled(!splitPossible);
                            if (ImGui::Selectable("Split key (interp, new arrange)")) {
                                CellAnim::AnimationKey newKey = key;
                                CellAnim::Arrangement newArrangement = *arrangementA;

                                unsigned maxParts = std::min(arrangementA->parts.size(), arrangementB->parts.size());
                                for (unsigned j = 0; j < maxParts; j++) {
                                    newArrangement.parts[j].transform =
                                        arrangementA->parts[j].transform.average(
                                            arrangementB->parts.at(j).transform
                                        );

                                    newArrangement.parts[j].opacity = AVERAGE_UCHARS(
                                        arrangementA->parts[j].opacity,
                                        arrangementB->parts.at(j).opacity
                                    );
                                }

                                arrangements.push_back(newArrangement);

                                CellAnim::AnimationKey modKey = key;

                                {
                                    newKey.arrangementIndex = arrangements.size() - 1;

                                    const auto& keyA = key;
                                    const auto& keyB = playerManager.getAnimation().keys.at(i+1);

                                    newKey.transform = keyA.transform.average(keyB.transform);

                                    newKey.opacity = AVERAGE_UCHARS(keyA.opacity, keyB.opacity);

                                    unsigned base = keyA.holdFrames - 1;

                                    unsigned first = base / 2;
                                    unsigned second = base - first;

                                    modKey.holdFrames = std::max(first, 1u);
                                    newKey.holdFrames = std::max(second, 1u);
                                }

                                sessionManager.getCurrentSession()->addCommand(
                                std::make_shared<CommandModifyAnimationKey>(
                                    sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                    playerManager.getAnimationIndex(),
                                    i,
                                    modKey
                                ));

                                playerManager.setKeyIndex(i + 1);

                                sessionManager.getCurrentSession()->addCommand(
                                std::make_shared<CommandInsertAnimationKey>(
                                    sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                    playerManager.getAnimationIndex(),
                                    i + 1,
                                    newKey
                                ));
                            }
                            ImGui::EndDisabled();
                        }

                        ImGui::Separator();

                        if (ImGui::Selectable(!io.KeyAlt ? "Push key after" : "Duplicate key after")) {
                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandInsertAnimationKey>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                i + 1,
                                io.KeyAlt ? key : CellAnim::AnimationKey()
                            ));

                            playerManager.setKeyIndex(i + 1);
                        }

                        if (ImGui::Selectable(!io.KeyAlt ? "Push key before" : "Duplicate key before")) {
                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandInsertAnimationKey>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                i,
                                io.KeyAlt ? key : CellAnim::AnimationKey()
                            ));

                            playerManager.setKeyIndex(i);
                        }

                        ImGui::Separator();

                        ImGui::BeginDisabled(i == (playerManager.getKeyCount() - 1));
                        if (ImGui::Selectable(!io.KeyAlt ? "Move up" : "Move up (without hold frames)")) {
                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandMoveAnimationKey>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                i,
                                false,
                                io.KeyAlt
                            ));

                            playerManager.setKeyIndex(playerManager.getKeyIndex() + 1);
                        }
                        ImGui::EndDisabled();

                        ImGui::BeginDisabled(i == 0);
                        if (ImGui::Selectable(!io.KeyAlt ? "Move back" : "Move back (without hold frames)")) {
                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandMoveAnimationKey>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                i,
                                true,
                                io.KeyAlt
                            ));

                            playerManager.setKeyIndex(playerManager.getKeyIndex() - 1);
                        }
                        ImGui::EndDisabled();

                        ImGui::Separator();

                        ImGui::BeginDisabled(playerManager.getKeyCount() == 1);
                        if (ImGui::Selectable("Delete key", false))
                            deleteKeyMode = DeleteKeyMode_Current;
                        ImGui::EndDisabled();

                        ImGui::Separator();

                        ImGui::BeginDisabled(i == 0);
                        if (ImGui::Selectable("Delete key(s) to left", false))
                            deleteKeyMode = DeleteKeyMode_ToLeft;
                        ImGui::EndDisabled();

                        ImGui::BeginDisabled(i == playerManager.getKeyCount() - 1);
                        if (ImGui::Selectable("Delete key(s) to right", false))
                            deleteKeyMode = DeleteKeyMode_ToRight;
                        ImGui::EndDisabled();

                        ImGui::EndPopup();
                    }

                    if (!appState.getArrangementMode() && ImGui::BeginItemTooltip()) {
                        ImGui::Text((const char*)ICON_FA_KEY "  Key no. %u", i+1);
                        ImGui::TextUnformatted("Right-click for options");

                        ImGui::Separator();

                        ImGui::BulletText("Arrangement Index: %u", key.arrangementIndex);
                        ImGui::Dummy({ 0.f, 10.f });
                        ImGui::BulletText("Held for: %u frame(s)", key.holdFrames);
                        ImGui::Dummy({ 0.f, 10.f });
                        ImGui::BulletText("Scale X: %f", key.transform.scaleX);
                        ImGui::BulletText("Scale Y: %f", key.transform.scaleY);
                        ImGui::BulletText("Angle: %f", key.transform.angle);
                        ImGui::Dummy({ 0.f, 10.f });
                        ImGui::BulletText("Opacity: %u/255", key.opacity);

                        ImGui::EndTooltip();
                    }

                    if (popColor)
                        ImGui::PopStyleColor();

                    // Hold frame dummy
                    if (key.holdFrames > 1) {
                        ImGui::SameLine();
                        ImGui::Dummy({ holdFrameWidth * key.holdFrames, buttonDimensions.y });
                    }

                    ImGui::SameLine();

                    ImGui::PopID();

                    if (deleteKeyMode) {
                        switch (deleteKeyMode) {
                        case DeleteKeyMode_Current: {
                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandDeleteAnimationKey>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                i
                            ));
                        } break;

                        case DeleteKeyMode_ToLeft: {
                            // Copy
                            CellAnim::Animation newAnimation = playerManager.getAnimation();
                            newAnimation.keys.erase(newAnimation.keys.begin(), newAnimation.keys.begin() + i);

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyAnimation>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                newAnimation
                            ));
                        } break;
                        case DeleteKeyMode_ToRight: {
                            // Copy
                            CellAnim::Animation newAnimation = playerManager.getAnimation();
                            newAnimation.keys.erase(newAnimation.keys.begin() + i + 1, newAnimation.keys.end());

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyAnimation>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getAnimationIndex(),
                                newAnimation
                            ));
                        } break;

                        default:
                            break;
                        }
                    }
                }

                ImGui::PopStyleVar(3);
            }

            // Hold Frames
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::Dummy({ 5.f, 0.f });
            ImGui::SameLine();
            ImGui::TextUnformatted("Hold Frames");

            ImGui::TableSetColumnIndex(1);

            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 3.f });
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.f, 4.f });
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

                for (unsigned i = 0; i < playerManager.getKeyCount(); i++) {
                    ImGui::PushID(i);

                    // Key button dummy
                    ImGui::Dummy(buttonDimensions);

                    unsigned holdFrames = playerManager.getAnimation().keys.at(i).holdFrames;
                    if (holdFrames > 1) {
                        ImGui::SameLine();

                        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {
                            playerManager.getKeyIndex() == i ?
                                ((holdFrames - playerManager.getHoldFramesLeft()) / (float)holdFrames) :
                            playerManager.getKeyIndex() > i ?
                                1.f : 0.f,
                            .5f
                        });

                        ImGui::BeginDisabled();
                        ImGui::Button((const char*)ICON_FA_HOURGLASS "", { holdFrameWidth * holdFrames, 30.f });
                        ImGui::EndDisabled();

                        ImGui::PopStyleVar();
                    }

                    ImGui::SameLine();

                    ImGui::PopID();
                }

                ImGui::PopStyleVar(3);
            }

            // Onion Skin
            if (appState.onionSkinState.enabled) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::Dummy({ 5.f, 0.f });
                ImGui::SameLine();
                ImGui::TextUnformatted("Onion Skin");

                ImGui::TableSetColumnIndex(1);

                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 3.f });
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.f, 4.f });
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

                    for (unsigned i = 0; i < playerManager.getKeyCount(); i++) {
                        ImGui::PushID(i);

                        char buffer[24];
                        snprintf(buffer, sizeof(buffer), "%u##OnionSkinButton", i + 1);

                        int keyCount = playerManager.getKeyCount();
                        int currentKeyIndex = playerManager.getKeyIndex();
                        int backStart = currentKeyIndex - appState.onionSkinState.backCount;
                        int frontEnd = currentKeyIndex + appState.onionSkinState.frontCount;

                        bool isOnionSkinFrame = false;
                        if (appState.onionSkinState.rollOver) {
                            unsigned wrappedBackStart = (backStart % keyCount + keyCount) % keyCount;
                            unsigned wrappedFrontEnd = frontEnd % keyCount;

                            // Normal range
                            if (wrappedBackStart <= wrappedFrontEnd)
                                isOnionSkinFrame = (i >= wrappedBackStart && i <= wrappedFrontEnd);
                            // Rollover range
                            else
                                isOnionSkinFrame = (i >= wrappedBackStart || i <= wrappedFrontEnd);
                        }
                        // Check regular bounds
                        else {
                            isOnionSkinFrame =
                                (i >= std::max<unsigned>(0, backStart) &&
                                i <= std::min<unsigned>(frontEnd, keyCount - 1));
                        }

                        ImGui::BeginDisabled();
                        if (isOnionSkinFrame && i != static_cast<unsigned>(currentKeyIndex))
                            ImGui::Button(buffer, buttonDimensions);
                        else
                            ImGui::Dummy(buttonDimensions);
                        ImGui::EndDisabled();

                        // Hold frame dummy
                        unsigned holdFrames = playerManager.getAnimation().keys.at(i).holdFrames;
                        if (holdFrames > 1) {
                            ImGui::SameLine();
                            ImGui::Dummy({ holdFrameWidth * holdFrames, buttonDimensions.y });
                        }

                        ImGui::SameLine();

                        ImGui::PopID();
                    }

                    ImGui::PopStyleVar(3);
                }
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::EndDisabled(); // arrangementMode

    ImGui::End();
}
