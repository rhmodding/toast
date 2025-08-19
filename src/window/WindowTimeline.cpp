#include "WindowTimeline.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <limits>

#include <cstdint>

#include "font/FontAwesome.h"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

#include "command/CommandMoveAnimationKey.hpp"
#include "command/CommandDeleteAnimationKey.hpp"
#include "command/CommandInsertAnimationKey.hpp"
#include "command/CommandModifyAnimationKey.hpp"
#include "command/CommandModifyAnimation.hpp"
#include "command/CommandModifyArrangements.hpp"

#include "command/CompositeCommand.hpp"

#include "util/TweenAnimUtil.hpp"

static const uint32_t u32_one = 1;

static const uint8_t  u8_min  = 0;
static const uint8_t  u8_max  = 0xFFu;

static ImU32 itemButtonColorU32() {
    const bool held    = ImGui::IsItemHovered() && ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered() && (ImGui::GetActiveID() !=  ImGui::GetCurrentWindowRead()->MoveId);

    const ImU32 color = ImGui::GetColorU32(
        (held & hovered) ? ImGuiCol_ButtonActive
        : hovered ? ImGuiCol_ButtonHovered
        : ImGuiCol_Button
    );

    return color;
}

static bool SplitButton(const char *strId, const char *label, const ImVec2 &sizeArg = ImVec2(0, 0)) {
    if (ImGui::GetCurrentWindowRead()->SkipItems)
        return false;

    ImDrawList &drawList = *ImGui::GetCurrentWindow()->DrawList;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float fontSize    = ImGui::GetFontSize();
    const ImVec2 groupPos   = ImGui::GetCursorScreenPos();

    ImGui::BeginGroup();
    ImGui::PushID(strId);

    bool togglePressed = false;
    float buttonHeight = 0.0;
    {
        const ImVec2 pos     = ImGui::GetCursorScreenPos();
        const ImVec2 minSize = ImGui::CalcTextSize(label, NULL, true) + style.FramePadding * 2.0f;
        const ImVec2 size    = ImGui::CalcItemSize(sizeArg, minSize.x, minSize.y);
        buttonHeight = size.y;

        togglePressed = ImGui::InvisibleButton("toggle", size, ImGuiButtonFlags_EnableNav);

        const ImRect bb(pos, pos + size);
        drawList.AddRectFilled(
            bb.Min, bb.Max,
            itemButtonColorU32(),
            style.FrameRounding, ImDrawFlags_RoundCornersLeft
        );
        drawList.AddText(pos + style.FramePadding, ImGui::GetColorU32(ImGuiCol_Text), label);
    }

    bool popupOpen = ImGui::IsPopupOpen("");
    {
        ImGui::SameLine(0.0, 1.0);
        const ImVec2 pos  = ImGui::GetCursorScreenPos();
        const ImVec2 size = ImVec2(buttonHeight, buttonHeight);

        bool pressed = ImGui::InvisibleButton("dropdown", size, ImGuiButtonFlags_EnableNav);

        if (pressed && !popupOpen) {
            ImGui::OpenPopup("");
            popupOpen = true;
        }

        const ImRect bb(pos, pos + size);
        drawList.AddRectFilled(
            bb.Min, bb.Max,
            popupOpen ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : itemButtonColorU32(),
            style.FrameRounding, ImDrawFlags_RoundCornersRight
        );

        ImVec2 mid = bb.Min + ImVec2(
            ImMax(0.0f, (size.x - fontSize) * 0.5f),
            ImMax(0.0f, (size.y - fontSize) * 0.5f)
        );

        ImGui::RenderArrow(&drawList, mid, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Down);
    }

    ImGui::PopID();
    ImGui::EndGroup();
    ImVec2 groupSize = ImGui::GetItemRectSize();

    if (popupOpen) {
        char name[20];
        ImFormatString(name, IM_ARRAYSIZE(name), "##Popup_%08x", ImGui::GetCurrentWindow()->GetID(strId));

        if (ImGuiWindow *popupWindow = ImGui::FindWindowByName(name)) {
            const ImRect bb(groupPos, groupPos + groupSize);

            popupWindow->AutoPosLastDirection = ImGuiDir_Down;
            ImGui::SetNextWindowPos(ImGui::FindBestWindowPosForPopupEx(
                bb.GetBL(),
                ImGui::CalcWindowNextAutoFitSize(popupWindow),
                &popupWindow->AutoPosLastDirection,
                ImGui::GetPopupAllowedExtentRect(popupWindow),
                bb,
                ImGuiPopupPositionPolicy_ComboBox
            ));
        }
    }

    return togglePressed;
}

void WindowTimeline::ChildToolbar() {
    PlayerManager& playerManager = PlayerManager::getInstance();

    ImGui::BeginChild("TimelineToolbar", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::Dummy({ 0.f, .5f });

    ImGui::Dummy({ 2.f, 0.f });
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 15.f, 0.f });

    if (ImGui::BeginTable("Table", 3, ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        char playPauseLabel[24] { '\0' };
        const char* playPauseIcon = playerManager.getPlaying() ?
            (const char*)ICON_FA_PAUSE : (const char*)ICON_FA_PLAY;

        std::snprintf(
            playPauseLabel, sizeof(playPauseLabel),
            "%s##PlayPause", playPauseIcon
        );

        constexpr ImVec2 normalButtonSize { 32, 32 };
        constexpr ImVec2 smallButtonSize  { 32 - 6, 32 - 6 };

        if (ImGui::Button(playPauseLabel, normalButtonSize)) {
            if (
                !playerManager.getPlaying() &&
                (playerManager.getKeyIndex() == playerManager.getKeyCount() - 1) &&
                (playerManager.getHoldFramesLeft() <= 1)
            ) {
                playerManager.setKeyIndex(0);
            }

            playerManager.ResetTimer();
            playerManager.setPlaying(!playerManager.getPlaying());
        }
        ImGui::SameLine();

        ImGui::Dummy({ 2.f, 0.f });
        ImGui::SameLine();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
        if (ImGui::Button((const char*)ICON_FA_BACKWARD_FAST "##GotoFirstKey", smallButtonSize)) {
            playerManager.setKeyIndex(0);
        }
        ImGui::SameLine();

        ImGui::SetItemTooltip("Go to first key");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
        if (ImGui::Button((const char*)ICON_FA_BACKWARD_STEP "##StepBackKey", smallButtonSize)) {
            if (playerManager.getKeyIndex() >= 1)
                playerManager.setKeyIndex(playerManager.getKeyIndex() - 1);
        }
        ImGui::SameLine();

        ImGui::SetItemTooltip("Step back a key");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
        if (ImGui::Button((const char*)ICON_FA_STOP "##Stop", smallButtonSize)) {
            playerManager.setPlaying(false);
            playerManager.setKeyIndex(0);
        } ImGui::SameLine();

        ImGui::SetItemTooltip("Stop playback and go to first key");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
        if (ImGui::Button((const char*)ICON_FA_FORWARD_STEP "##StepForwKey", smallButtonSize)) {
            if (playerManager.getKeyIndex() != playerManager.getKeyCount() - 1) {
                playerManager.setKeyIndex(playerManager.getKeyIndex() + 1);
            }
        } ImGui::SameLine();

        ImGui::SetItemTooltip("Step forward a key");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
        if (ImGui::Button((const char*)ICON_FA_FORWARD_FAST "##GotoLastKey", smallButtonSize)) {
            playerManager.setKeyIndex(playerManager.getKeyCount() - 1);
        } ImGui::SameLine();

        ImGui::SetItemTooltip("Go to last key");

        ImGui::Dummy({ 2.f, 0.f });
        ImGui::SameLine();

        {
            const bool isLooping = playerManager.getLooping();
            if (isLooping)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

            if (ImGui::Button((const char*)ICON_FA_ARROW_ROTATE_RIGHT "##ToggleLoop", normalButtonSize))
                playerManager.setLooping(!isLooping);

            ImGui::SetItemTooltip("Toggle looping");

            if (isLooping)
                ImGui::PopStyleColor();
        }

        ImGui::TableSetColumnIndex(1);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);

        int keyNo = playerManager.getKeyIndex() + 1;

        ImGui::SetNextItemWidth(52.f);
        if (ImGui::InputInt("Key No.", &keyNo, 0, 0)) {
            playerManager.setKeyIndex(std::clamp<unsigned>(keyNo - 1, 0, playerManager.getKeyCount() - 1));
        }

        ImGui::SameLine();

        int frameNo = std::min<int>(
            playerManager.getElapsedFrames() + 1,
            playerManager.getTotalFrames()
        );

        ImGui::SetNextItemWidth(52.f);
        if (ImGui::InputInt("Frame No.", &frameNo, 0, 0)) {
            playerManager.setElapsedFrames(std::max<int>(0, frameNo - 1));
        }

        ImGui::SameLine();

        int frameRate = playerManager.getFrameRate();

        ImGui::SetNextItemWidth(52.f);
        if (ImGui::InputInt("FPS", &frameRate, 0, 0)) {
            playerManager.setFrameRate(std::max<int>(frameRate, 1));
        }

        ImGui::TableSetColumnIndex(2);

        OnionSkinState& onionSkinState = playerManager.getOnionSkinState();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);

        if (SplitButton("###OnionSkinOptions", (const char*) ICON_FA_EYE " Onion Skin", { 0.f, 32.f - 6.f }))
            onionSkinState.enabled ^= true;

        if (ImGui::BeginPopup("###OnionSkinOptions")) {
            ImGui::InputScalar("Back count", ImGuiDataType_U32, &onionSkinState.backCount, &u32_one);
            ImGui::InputScalar("Front count", ImGuiDataType_U32, &onionSkinState.frontCount, &u32_one);

            ImGui::Separator();

            ImGui::DragScalar(
                "Opacity",
                ImGuiDataType_U8, &onionSkinState.opacity,
                1.f, &u8_min, &u8_max,
                "%u/255",
                ImGuiSliderFlags_AlwaysClamp
            );

            ImGui::Separator();

            ImGui::Checkbox("Draw behind", &onionSkinState.drawUnder);

            ImGui::Checkbox("Roll over (loop)", &onionSkinState.rollOver);

            ImGui::EndPopup();
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();

    ImGui::Dummy({ 0.f, .5f });

    ImGui::EndChild();
}

static void drawFrameIndic(float height, float keyWidth, float holdWidth, float keySpacing) {
    PlayerManager& playerManager = PlayerManager::getInstance();
    const auto& animation = playerManager.getAnimation();

    unsigned currentFrame = 1;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.f, 0.f });
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    ImGui::BeginGroup();

    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float cursorX = startPos.x;

    for (unsigned i = 0; i < playerManager.getKeyCount(); ++i) {
        unsigned keyDuration = animation.keys[i].holdFrames;

        if (keyDuration == 0) {
            cursorX += keyWidth + keySpacing;
            continue;
        }

        for (unsigned j = 0; j < keyDuration; ++j) {
            float width = (j == 0) ? keyWidth : holdWidth;
            float spacing = (j == 0 || j == (keyDuration - 1)) ? keySpacing : 0.f;

            char textBuf[16];
            std::snprintf(textBuf, sizeof(textBuf), "%u", currentFrame++);
            ImVec2 textSize = ImGui::CalcTextSize(textBuf);

            float textX = cursorX + (width - textSize.x) / 2.0f;
            float textY = startPos.y + (height - textSize.y) / 2.0f;

            ImGui::SetCursorScreenPos({ cursorX, startPos.y });
            ImGui::Dummy({ width, height });

            drawList->AddText({ textX, textY }, ImGui::GetColorU32(ImGuiCol_TextDisabled), textBuf);


            cursorX += width + spacing;
        }
    }

    ImGui::EndGroup();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void WindowTimeline::ChildKeys() {
    PlayerManager& playerManager = PlayerManager::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();

    ImGui::BeginChild("TimelineKeys", { 0.f, 0.f }, 0, ImGuiWindowFlags_HorizontalScrollbar);

    constexpr ImVec2 buttonSize { 22.f, 30.f };

    constexpr float holdFrameWidth { buttonSize.x / 1.f };

    constexpr float keySpacing = 2.f;

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 15.f, 0.f });

    if (ImGui::BeginTable(
        "TimelineFrameTable", 2,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollX
    )) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::Dummy({ 5.f, 0.f });
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted("Frames");
        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(1);

        drawFrameIndic(15.f, buttonSize.x, holdFrameWidth, keySpacing);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::Dummy({ 5.f, 0.f });
        ImGui::SameLine();
        ImGui::TextUnformatted("Keys");

        ImGui::TableSetColumnIndex(1);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 3.f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { keySpacing, 4.f });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

        for (unsigned i = 0; i < playerManager.getKeyCount(); i++) {
            ImGui::PushID(i);

            bool popColor { false };
            if (playerManager.getKeyIndex() == i) {
                popColor = true;
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            char buttonLabel[24] { '\0' };
            std::snprintf(
                buttonLabel, sizeof(buttonLabel),
                "%u##KeyButton", i+1
            );

            const CellAnim::AnimationKey& key = playerManager.getAnimation().keys.at(i);

            bool buttonIsTransparent = key.holdFrames == 0;
            if (buttonIsTransparent) {
                const auto* guiContext = ImGui::GetCurrentContext();
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, guiContext->Style.Alpha * guiContext->Style.DisabledAlpha);
            }

            if (ImGui::Button(buttonLabel, buttonSize))
                playerManager.setKeyIndex(i);

            if (buttonIsTransparent)
                ImGui::PopStyleVar();

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
                // TODO: interp util??
                {
                    bool splitPossible { false };
                    const CellAnim::Arrangement* arrangementA { nullptr };
                    const CellAnim::Arrangement* arrangementB { nullptr };

                    auto& arrangements =
                        sessionManager.getCurrentSession()
                            ->getCurrentCellAnim().object->getArrangements();

                    if (i+1 < playerManager.getKeyCount()) {
                        splitPossible = true;
                    }

                    ImGui::BeginDisabled(!splitPossible);
                    if (ImGui::Selectable("Split key (interp, new arrange)")) {
                        auto newArrangements = arrangements;
                        auto newAnimation = playerManager.getAnimation();

                        CellAnim::AnimationKey& k0 = newAnimation.keys.at(i);
                        const CellAnim::AnimationKey& k1 = newAnimation.keys.at(i + 1);

                        unsigned totalDuration = k0.holdFrames;

                        unsigned firstSegment = totalDuration / 2;
                        unsigned secondSegment = totalDuration - firstSegment;

                        float tweenT = static_cast<float>(firstSegment) / totalDuration;

                        k0.holdFrames = firstSegment;

                        CellAnim::AnimationKey newKey = TweenAnimUtil::tweenAnimKeys(newArrangements, k0, k1, tweenT);
                        newKey.holdFrames = secondSegment;

                        newAnimation.keys.insert(newAnimation.keys.begin() + i + 1, newKey);

                        auto composite = std::make_shared<CompositeCommand>();

                        composite->addCommand(std::make_shared<CommandModifyArrangements>(
                            sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                            newArrangements
                        ));
                        composite->addCommand(std::make_shared<CommandModifyAnimation>(
                            sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                            playerManager.getAnimationIndex(),
                            newAnimation
                        ));

                        sessionManager.getCurrentSession()->addCommand(composite);

                        playerManager.setKeyIndex(i + 1);
                    }
                    ImGui::EndDisabled();
                }

                ImGui::Separator();

                bool notHoldAlt = !ImGui::GetIO().KeyAlt; 

                if (ImGui::Selectable(notHoldAlt ? "Push key after" : "Duplicate key after")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        i + 1,
                        notHoldAlt ? CellAnim::AnimationKey() : key
                    ));

                    playerManager.setKeyIndex(i + 1);
                }

                if (ImGui::Selectable(notHoldAlt ? "Push key before" : "Duplicate key before")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandInsertAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        i,
                        notHoldAlt ? CellAnim::AnimationKey() : key
                    ));

                    playerManager.setKeyIndex(i);
                }

                ImGui::Separator();

                ImGui::BeginDisabled(i == (playerManager.getKeyCount() - 1));
                if (ImGui::Selectable(notHoldAlt ? "Move up" : "Move up (without hold frames)")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        i,
                        false,
                        !notHoldAlt
                    ));

                    playerManager.setKeyIndex(playerManager.getKeyIndex() + 1);
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(i == 0);
                if (ImGui::Selectable(notHoldAlt ? "Move back" : "Move back (without hold frames)")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        i,
                        true,
                        !notHoldAlt
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

            // For some reason, tooltips still activate when disabled ..
            bool isDisabled = ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled;
            if (!isDisabled && ImGui::BeginItemTooltip()) {
                ImGui::Text((const char*)ICON_FA_KEY "  Key no. %u", i+1);
                ImGui::TextUnformatted("Right-click for options");

                if (key.holdFrames == 0) {
                    ImGui::BulletText("This key will be skipped during\nplayback (held for 0 frames).");
                    ImGui::Dummy({ 0.f, 5.f });
                }

                ImGui::BulletText("Arrangement No.: %u", key.arrangementIndex + 1);
                ImGui::Dummy({ 0.f, 10.f });
                ImGui::BulletText("Held for: %u frame(s)", key.holdFrames);
                ImGui::Dummy({ 0.f, 10.f });
                ImGui::BulletText("Scale X: %f", key.transform.scale.x);
                ImGui::BulletText("Scale Y: %f", key.transform.scale.y);
                ImGui::BulletText("Angle: %f", key.transform.angle);
                ImGui::Dummy({ 0.f, 10.f });
                ImGui::BulletText("Opacity: %u/255", key.opacity);

                ImGui::EndTooltip();
            }

            if (popColor)
                ImGui::PopStyleColor();

            // Spacing for hold frame(s).
            if (key.holdFrames > 1) {
                ImGui::SameLine();
                ImGui::Dummy({ holdFrameWidth * (key.holdFrames - 1), buttonSize.y });
            }

            ImGui::SameLine();

            ImGui::PopID();

            if (deleteKeyMode) {
                switch (deleteKeyMode) {
                case DeleteKeyMode_Current: {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandDeleteAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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

        // Hold Frames
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::Dummy({ 5.f, 0.f });
        ImGui::SameLine();
        ImGui::TextUnformatted("Hold Frames");

        ImGui::TableSetColumnIndex(1);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 3.f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { keySpacing, 4.f });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

        unsigned currentKeyIndex = playerManager.getKeyIndex();

        for (unsigned i = 0; i < playerManager.getKeyCount(); i++) {
            ImGui::PushID(i);

            // Spacing for key(s).
            ImGui::Dummy(buttonSize);

            unsigned duration = playerManager.getAnimation().keys[i].holdFrames;
            if (duration > 1) {
                ImGui::SameLine();

                bool styledColor = false;

                // Move icon along frame.
                if (i == currentKeyIndex) {
                    float framesPass = duration - playerManager.getHoldFramesLeft();
                    float pass = std::max((framesPass - 1.0f) / (duration - 2.0f), 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(pass, 0.5f));
                }
                else if (i < currentKeyIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    styledColor = true;

                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(1.0f, 0.5f));
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    styledColor = true;

                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
                }

                ImGui::BeginDisabled();
                ImGui::Button((const char*)ICON_FA_HOURGLASS "", { holdFrameWidth * (duration - 1), 30.f });
                ImGui::EndDisabled();

                ImGui::PopStyleVar();
                if (styledColor)
                    ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            ImGui::PopID();
        }

        ImGui::PopStyleVar(3);

        const OnionSkinState& onionSkinState = playerManager.getOnionSkinState();
        if (onionSkinState.enabled) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::Dummy({ 5.f, 0.f });
            ImGui::SameLine();
            ImGui::TextUnformatted("Onion Skin");

            ImGui::TableSetColumnIndex(1);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 3.f });
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.f, 4.f });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

            for (int i = 0; i < playerManager.getKeyCount(); i++) {
                ImGui::PushID(i);

                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "%u##OnionSkinButton", i + 1);

                int keyCount = playerManager.getKeyCount();
                int currentKeyIndex = playerManager.getKeyIndex();
                int backStart = currentKeyIndex - onionSkinState.backCount;
                int frontEnd = currentKeyIndex + onionSkinState.frontCount;

                bool isOnionSkinFrame = false;
                if (onionSkinState.rollOver) {
                    unsigned wrappedBackStart = (backStart % keyCount + keyCount) % keyCount;
                    unsigned wrappedFrontEnd = frontEnd % keyCount;

                    // In-bounds range.
                    if (wrappedBackStart <= wrappedFrontEnd) {
                        isOnionSkinFrame = (i >= wrappedBackStart && i <= wrappedFrontEnd);
                    }
                    // Rollover range.
                    else {
                        isOnionSkinFrame = (i >= wrappedBackStart || i <= wrappedFrontEnd);
                    }
                }
                // Check regular bounds.
                else {
                    isOnionSkinFrame = (
                        i >= std::max<int>(0, backStart) &&
                        i <= std::min<int>(frontEnd, keyCount - 1)
                    );
                }

                ImGui::BeginDisabled();
                if (isOnionSkinFrame && i != static_cast<unsigned>(currentKeyIndex))
                    ImGui::Button(buffer, buttonSize);
                else
                    ImGui::Dummy(buttonSize);
                ImGui::EndDisabled();

                // Spacing for hold frame(s).
                unsigned holdFrames = playerManager.getAnimation().keys[i].holdFrames;
                if (holdFrames > 1) {
                    ImGui::SameLine();
                    ImGui::Dummy({ holdFrameWidth * (holdFrames - 1), buttonSize.y });
                }

                ImGui::SameLine();

                ImGui::PopID();
            }

            ImGui::PopStyleVar(3);
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();

    ImGui::EndChild();
}

// TODO: this whole thing is in desperate need of a refactor ...
void WindowTimeline::Update() {
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    const ImGuiIO& io = ImGui::GetIO(); 

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::Begin("Timeline");
    ImGui::PopStyleVar();

    bool arrangementMode = sessionManager.getCurrentSession()->arrangementMode;

    ImGui::BeginDisabled(arrangementMode);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);

    ChildToolbar();
    ChildKeys();

    ImGui::EndDisabled();

    ImGui::End();
}
