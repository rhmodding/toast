#include "WindowTimeline.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdint>

#include "../AppState.hpp"

#include "../font/FontAwesome.h"

void WindowTimeline::Update() {
    GET_APP_STATE;
    GET_ANIMATABLE;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Timeline");
    ImGui::PopStyleVar();

    if (appState.arrangementMode)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));

    ImGui::BeginChild("TimelineToolbar", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 0.5f));
        
        ImGui::Dummy(ImVec2(2, 0));
        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(15, 0));

        if (ImGui::BeginTable("TimelineToolbarTable", 2, ImGuiTableFlags_BordersInnerV)) {
            for (uint16_t row = 0; row < 1; row++) {
                ImGui::TableNextRow();
                for (uint16_t column = 0; column < 2; column++) {
                    ImGui::TableSetColumnIndex(column);

                    switch (column) {
                        case 0: {
                            char playPauseButtonLabel[24] = { '\0' };
                            const char* playPauseIcon = appState.playerState.playing ? (char*)ICON_FA_PAUSE : (char*)ICON_FA_PLAY;

                            sprintf(playPauseButtonLabel, "%s##playPauseButton", playPauseIcon);

                            if (ImGui::Button(playPauseButtonLabel, ImVec2(32, 32))) {
                                if (
                                    (appState.playerState.currentFrame == appState.playerState.frameCount - 1) &&
                                    (appState.playerState.holdFramesLeft == 0)
                                ) {
                                    appState.playerState.currentFrame = 0;
                                    appState.playerState.updateCurrentFrame();
                                }

                                appState.playerState.ResetTimer();
                                appState.playerState.ToggleAnimating(!appState.playerState.playing);
                            } 
                            ImGui::SameLine();

                            ImGui::Dummy(ImVec2(2, 0));
                            ImGui::SameLine();

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_BACKWARD_FAST "##firstFrameButton", ImVec2(32-6, 32-6))) {
                                appState.playerState.currentFrame = 0;
                                appState.playerState.updateCurrentFrame();
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Go to first key");

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_BACKWARD_STEP "##backFrameButton", ImVec2(32-6, 32-6))) {
                                if (appState.playerState.currentFrame >= 1) {
                                    appState.playerState.currentFrame--;
                                    appState.playerState.updateCurrentFrame();
                                }
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Step back a key");
                            
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_STOP "##stopButton", ImVec2(32-6, 32-6))) {
                                appState.playerState.playing = false;
                                appState.playerState.currentFrame = 0;
                                appState.playerState.updateCurrentFrame();
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Stop playback and go to first key");

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_FORWARD_STEP "##forwardFrameButton", ImVec2(32-6, 32-6))) {
                                if (appState.playerState.currentFrame < appState.playerState.frameCount - 1) {
                                    appState.playerState.currentFrame++;
                                    appState.playerState.updateCurrentFrame();
                                }
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Step forward a key");
                            
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_FORWARD_FAST "##lastFrameButton", ImVec2(32-6, 32-6))) {
                                appState.playerState.currentFrame =
                                    appState.playerState.frameCount -
                                    (appState.playerState.frameCount > 0 ? 1 : 0);

                                appState.playerState.updateCurrentFrame();
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Go to last key");

                            ImGui::Dummy(ImVec2(2, 0));
                            ImGui::SameLine();

                            {
                                bool popColor{ false }; // Use local boolean since state can be mutated by button
                                if (appState.playerState.loopEnabled) {
                                    popColor = true;
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                                }

                                if (ImGui::Button((char*)ICON_FA_ARROW_ROTATE_RIGHT, ImVec2(32, 32)))
                                    appState.playerState.loopEnabled = !appState.playerState.loopEnabled;

                                ImGui::SetItemTooltip("Toggle looping");

                                if (popColor)
                                    ImGui::PopStyleColor();
                            }
                        } break;

                        case 1: {
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);

                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                            if (ImGui::InputScalar("Frame", ImGuiDataType_U16, &appState.playerState.currentFrame, nullptr, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue)) {
                                appState.playerState.updateCurrentFrame();
                            }

                            ImGui::SameLine();

                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::InputScalar("Hold Frames Left", ImGuiDataType_U16, &appState.playerState.holdFramesLeft, nullptr, nullptr, "%u", ImGuiInputTextFlags_ReadOnly);
                            ImGui::PopItemFlag();

                            ImGui::SameLine();

                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                            ImGui::InputScalar("FPS", ImGuiDataType_U16, &appState.playerState.frameRate, nullptr, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue);
                        } break;

                        case 2: {
                            /*
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);

                            ImGui::Checkbox("Auto-scroll timeline", &autoScrollTimeline);
                            */
                        } break;

                        default: {

                        } break;
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        ImGui::Dummy(ImVec2(0, 0.5f));
    }
    ImGui::EndChild();

    ImGui::BeginChild("TimelineKeys", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar);
    {
        const ImVec2 buttonDimensions(22.f, 30.f);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(15, 0));
        
        if (ImGui::BeginTable("TimelineFrameTable", 2,
                                                        ImGuiTableFlags_RowBg |
                                                        ImGuiTableFlags_BordersInnerV |
                                                        ImGuiTableFlags_ScrollX
        )) {
            ImGui::TableNextRow();
            {
                ImGui::TableSetColumnIndex(0);
                ImGui::Dummy(ImVec2(5, 0));
                ImGui::SameLine();
                ImGui::TextUnformatted("Keys");
                ImGui::TableSetColumnIndex(1);

                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);

                    for (uint16_t i = 0; i < appState.playerState.frameCount; i++) {
                        ImGui::PushID(i);

                        bool popColor{ false };
                        if (appState.playerState.currentFrame == i) {
                            popColor = true;
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        }

                        char buffer[18];
                        sprintf(buffer, "%u##KeyButton", i+1);

                        if (ImGui::Button(buffer, buttonDimensions)) {
                            appState.playerState.currentFrame = i;
                            appState.playerState.updateCurrentFrame();
                        }

                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text((char*)ICON_FA_KEY "  Key no. %u", i+1);

                            ImGui::Separator();

                            RvlCellAnim::AnimationKey* key = &globalAnimatable->getCurrentAnimation()->keys.at(i);

                            ImGui::BulletText("Arrangement Index: %u", key->arrangementIndex);
                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::BulletText("Held for: %u frame(s)", key->holdFrames);
                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::BulletText("Scale X: %f", key->scaleX);
                            ImGui::BulletText("Scale Y: %f", key->scaleY);
                            ImGui::BulletText("Angle: %f", key->angle);
                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::BulletText("Opacity: %u/255", key->opacity);

                            ImGui::EndTooltip();
                        }

                        if (popColor)
                            ImGui::PopStyleColor();

                        // Hold frame dummy

                        uint16_t holdFrames = globalAnimatable->getCurrentAnimation()->keys.at(i).holdFrames;
                        if (holdFrames > 1) {
                            ImGui::SameLine();
                            ImGui::Dummy(ImVec2(static_cast<float>(10 * holdFrames), buttonDimensions.y));
                        }

                        ImGui::SameLine();

                        ImGui::PopID();
                    }

                    ImGui::PopStyleVar(3);
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableSetColumnIndex(0);
                ImGui::Dummy(ImVec2(5, 0));
                ImGui::SameLine();
                ImGui::TextUnformatted("Hold Frames");
                ImGui::TableSetColumnIndex(1);

                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);

                    for (uint16_t i = 0; i < appState.playerState.frameCount; i++) {
                        ImGui::PushID(i);

                        // Key button dummy
                        ImGui::Dummy(buttonDimensions);

                        uint16_t holdFrames = globalAnimatable->getCurrentAnimation()->keys.at(i).holdFrames;
                        if (holdFrames > 1) {
                            ImGui::SameLine();

                            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(
                                appState.playerState.currentFrame == i ?
                                    ((holdFrames - appState.playerState.holdFramesLeft) / (float)holdFrames) :
                                appState.playerState.currentFrame > i ?
                                    1.0f : 0.0f,
                                0.5f
                            ));

                            ImGui::BeginDisabled();
                            ImGui::Button((char*)ICON_FA_HOURGLASS "", { static_cast<float>(10 * holdFrames), 30.f });
                            ImGui::EndDisabled();

                            ImGui::PopStyleVar();
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

        /*
        if (autoScrollTimeline && appState.playerState.playing) {
            ImGui::SetScrollX(ImGui::GetScrollMaxX() * appState.playerState.getAnimationProgression());
        }
        */
    }
    ImGui::EndChild();

    if (appState.arrangementMode)
        ImGui::EndDisabled();

    ImGui::End();
}