#include "WindowInspector.hpp"

#include "imgui.h"

#include "../font/FontAwesome.h"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

#include "../ConfigManager.hpp"

const uint8_t uint8_one = 1;
const uint16_t uint16_one = 1;
const uint32_t uint32_one = 1;

void DrawPreview(AppState& appState, Animatable* animatable) {
    GET_WINDOW_DRAWLIST;

    ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = { 92, 92 };
    ImVec2 canvasBottomRight = ImVec2(canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y);

    const ImVec2 origin(
        canvasTopLeft.x + static_cast<int>(canvasSize.x / 2),
        canvasTopLeft.y + static_cast<int>(canvasSize.y / 2)
    );

    uint32_t backgroundColor = appState.darkTheme ?
        IM_COL32(50, 50, 50, 255) :
        IM_COL32(255, 255, 255, 255);

    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor, ImGui::GetStyle().FrameRounding);

    drawList->PushClipRect(canvasTopLeft, canvasBottomRight, true);

    animatable->offset = origin;
    animatable->scaleX = 0.7f;
    animatable->scaleY = 0.7f;
    animatable->Draw(drawList);

    drawList->PopClipRect();

    ImGui::Dummy(canvasSize);
}

void WindowInspector::Level_Animation() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    DrawPreview(appState, globalAnimatable);

    ImGui::SameLine();

    uint16_t animationIndex = globalAnimatable->getCurrentAnimationIndex();
    auto query = sessionManager.getCurrentSession()->getAnimationNames()->find(animationIndex);

    const char* animationName = 
        query != sessionManager.getCurrentSession()->getAnimationNames()->end() ?
            query->second.c_str() : nullptr;

    ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

        ImGui::Text("Animation no. %u:", animationIndex);

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
        if (copyName.size() > 256)
            copyName.resize(256); // Truncate

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
            changed |= true;

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

void WindowInspector::Level_Key() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    DrawPreview(appState, globalAnimatable);

    ImGui::SameLine();

    uint16_t animationIndex = globalAnimatable->getCurrentAnimationIndex();
    auto query = sessionManager.getCurrentSession()->getAnimationNames()->find(animationIndex);

    const char* animationName = 
        query != sessionManager.getCurrentSession()->getAnimationNames()->end() ?
            query->second.c_str() : nullptr;

    ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

        ImGui::Text("Anim \"%s\" (no. %u)", animationName ? animationName : "no macro defined", animationIndex);

        ImGui::PushFont(appState.fontLarge);
        ImGui::TextWrapped("Key no. %u", globalAnimatable->getCurrentKeyIndex());
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    RvlCellAnim::AnimationKey* animKey = globalAnimatable->getCurrentKey();

    ImGui::SeparatorText((char*)ICON_FA_KEY " Properties");

    changed |= ImGui::InputScalar("Arrangement Index", ImGuiDataType_U16, &animKey->arrangementIndex, &uint16_one, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue);

    uint16_t holdFrames = animKey->holdFrames;
    if (ImGui::InputScalar("Hold Frames", ImGuiDataType_U16, &holdFrames, &uint16_one, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue)) {
        changed |= true;

        if (holdFrames <= 1)
            animKey->holdFrames = 1;
        else
            animKey->holdFrames = holdFrames;
    };

    ImGui::SeparatorText((char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform");

    float scaleValues[2] = { animKey->scaleX, animKey->scaleY };
    if (ImGui::DragFloat2("Scale XY", scaleValues, 0.01f)) {
        changed |= true;

        animKey->scaleX = scaleValues[0];
        animKey->scaleY = scaleValues[1];
    }
    changed |= ImGui::SliderFloat("Angle Z", &animKey->angle, -360.f, 360.f, "%.1f deg");

    ImGui::SeparatorText((char*)ICON_FA_IMAGE " Rendering");

    changed |= ImGui::InputScalar("Opacity", ImGuiDataType_U8, &animKey->opacity, &uint8_one, nullptr, "%u");

    if (ConfigManager::getInstance().config.showUnknownValues) {
        ImGui::SeparatorText((char*)ICON_FA_CIRCLE_QUESTION " Unknown value (byteswapped)..");

        if (ImGui::CollapsingHeader("..as Uint32", ImGuiTreeNodeFlags_None)) {
            changed |= ImGui::InputScalar(" A", ImGuiDataType_U32, &animKey->unknown.u32, &uint32_one, nullptr, "%u");
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("..as Uint16", ImGuiTreeNodeFlags_None)) {
            changed |= ImGui::InputScalar(" A", ImGuiDataType_U16, &animKey->unknown.u16[0], &uint16_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" B", ImGuiDataType_U16, &animKey->unknown.u16[1], &uint16_one, nullptr, "%u");
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("..as Uint8 (byte)", ImGuiTreeNodeFlags_None)) {
            changed |= ImGui::InputScalar(" A", ImGuiDataType_U8, &animKey->unknown.u8[0], &uint8_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" B", ImGuiDataType_U8, &animKey->unknown.u8[1], &uint8_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" C", ImGuiDataType_U8, &animKey->unknown.u8[2], &uint8_one, nullptr, "%u");
            changed |= ImGui::InputScalar(" D", ImGuiDataType_U8, &animKey->unknown.u8[3], &uint8_one, nullptr, "%u");
        }
    }
}

void WindowInspector::Level_Arrangement() {
    GET_APP_STATE;
    GET_ANIMATABLE;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

    ImGui::BeginChild("ArrangementOverview", { 0, (ImGui::GetWindowSize().y / 2.f) }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
    ImGui::PopStyleVar();
    {
        DrawPreview(appState, globalAnimatable);

        ImGui::SameLine();

        ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
        {
            //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

            ImGui::PushFont(appState.fontLarge);
            ImGui::TextWrapped("Arrangement no.");
            ImGui::PopFont();

            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
            ImGui::InputScalar(
                "##ArrangementInput",
                ImGuiDataType_U16,
                &globalAnimatable->getCurrentKey()->arrangementIndex,
                nullptr, nullptr,
                "%u"
            );

            const float buttonSize = ImGui::GetFrameHeight();
            const float xInnerSpacing = ImGui::GetStyle().ItemInnerSpacing.x;

            ImGui::PushButtonRepeat(true);

            ImGui::SameLine(0.f, xInnerSpacing);
            if (ImGui::Button("-##ArrangementInputSub", ImVec2(buttonSize, buttonSize))) {
                if (globalAnimatable->getCurrentKey()->arrangementIndex > 0){
                    globalAnimatable->getCurrentKey()->arrangementIndex--;

                    RvlCellAnim::Arrangement* arrangementPtr =
                        &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);
                }
            }
            ImGui::SameLine(0.f, xInnerSpacing);
            if (ImGui::Button("+##ArrangementInputAdd", ImVec2(buttonSize, buttonSize))) {
                if (globalAnimatable->getCurrentKey()->arrangementIndex < globalAnimatable->cellanim->arrangements.size()) {
                    globalAnimatable->getCurrentKey()->arrangementIndex++;

                    RvlCellAnim::Arrangement* arrangementPtr =
                        &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

                    if (appState.selectedPart >= arrangementPtr->parts.size())
                        appState.selectedPart = static_cast<int32_t>(arrangementPtr->parts.size() - 1);
                }
            }

            ImGui::PopButtonRepeat();
            
            //ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        RvlCellAnim::Arrangement* arrangementPtr =
            &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

        RvlCellAnim::ArrangementPart* partPtr = appState.selectedPart >= 0 ? &arrangementPtr->parts.at(appState.selectedPart) : nullptr;

        if (partPtr) {
            ImGui::SeparatorText((char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Part Transform");

            int positionValues[2] = {
                partPtr->positionX - (this->realPosition ? 0 : 512),
                partPtr->positionY - (this->realPosition ? 0 : 512)
            };
            if (ImGui::DragInt2("Position XY", positionValues, 1.f)) {
                changed |= true;

                partPtr->positionX = static_cast<int16_t>(
                    positionValues[0] + (this->realPosition ? 0 : 512)
                );
                partPtr->positionY = static_cast<int16_t>(
                    positionValues[1] + (this->realPosition ? 0 : 512)
                );
            }

            float scaleValues[2] = { partPtr->scaleX, partPtr->scaleY };
            if (ImGui::DragFloat2("Scale XY", scaleValues, 0.01f)) {
                changed |= true;

                partPtr->scaleX = scaleValues[0];
                partPtr->scaleY = scaleValues[1];
            }
            changed |= ImGui::SliderFloat("Angle Z", &partPtr->angle, -360.f, 360.f, "%.1f deg");

            changed |= ImGui::Checkbox("Flip X", &partPtr->flipX);
            changed |= ImGui::Checkbox("Flip Y", &partPtr->flipY);

            ImGui::SeparatorText((char*)ICON_FA_IMAGE " Rendering");

            changed |= ImGui::InputScalar("Opacity", ImGuiDataType_U8, &partPtr->opacity, &uint8_one, nullptr, "%u");

            if (ConfigManager::getInstance().config.showUnknownValues) {
                ImGui::SeparatorText((char*)ICON_FA_CIRCLE_QUESTION " Unknown value (byteswapped)..");

                if (ImGui::CollapsingHeader("..as Uint32", ImGuiTreeNodeFlags_None)) {
                    changed |= ImGui::InputScalar(" A", ImGuiDataType_U32, &partPtr->unknown.u32, &uint32_one, nullptr, "%u");
                }
                ImGui::Separator();
                if (ImGui::CollapsingHeader("..as Uint16", ImGuiTreeNodeFlags_None)) {
                    changed |= ImGui::InputScalar(" A", ImGuiDataType_U16, &partPtr->unknown.u16[0], &uint16_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" B", ImGuiDataType_U16, &partPtr->unknown.u16[1], &uint16_one, nullptr, "%u");
                }
                ImGui::Separator();
                if (ImGui::CollapsingHeader("..as Uint8 (byte)", ImGuiTreeNodeFlags_None)) {
                    changed |= ImGui::InputScalar(" A", ImGuiDataType_U8, &partPtr->unknown.u8[0], &uint8_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" B", ImGuiDataType_U8, &partPtr->unknown.u8[1], &uint8_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" C", ImGuiDataType_U8, &partPtr->unknown.u8[2], &uint8_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" D", ImGuiDataType_U8, &partPtr->unknown.u8[3], &uint8_one, nullptr, "%u");
                }
            }
        } else {
            ImGui::Text("No part selected.");
        }
    }
    ImGui::EndChild();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

    ImGui::BeginChild("ArrangementParts", { 0, 0 }, ImGuiChildFlags_Border);
    ImGui::PopStyleVar();
    {
        ImGui::SeparatorText((char*)ICON_FA_IMAGE " Parts");

        RvlCellAnim::Arrangement* arrangementPtr =
            &globalAnimatable->cellanim->arrangements.at(globalAnimatable->getCurrentKey()->arrangementIndex);

        // Use a signed 32-bit integer since n will eventually be a negative value
        for (int n = static_cast<int>(arrangementPtr->parts.size()) - 1; n >= 0; n--) {
            ImGui::PushID(n);

            char buffer[16];
            sprintf(buffer, "Part no. %u", n+1);

            ImGui::SetNextItemAllowOverlap();
            if (ImGui::Selectable(buffer, appState.selectedPart == n))
                appState.selectedPart = n;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6, ImGui::GetStyle().ItemSpacing.y });

            float firstButtonWidth = ImGui::CalcTextSize((char*) ICON_FA_ARROW_DOWN "").x + (ImGui::GetStyle().FramePadding.x * 2);
            float basePositionX = ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().WindowPadding.x - 10.f;

            ImGui::SameLine();
            ImGui::SetCursorPosX(basePositionX - firstButtonWidth - ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::SmallButton((char*) ICON_FA_ARROW_UP "")) {
                changed |= true;

                uint16_t nSwap = n + 1;
                if (nSwap >= 0 && nSwap < arrangementPtr->parts.size()) {
                    std::swap(arrangementPtr->parts.at(n), arrangementPtr->parts.at(nSwap));
                    appState.selectedPart = nSwap;
                }
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(basePositionX);
            if (ImGui::SmallButton((char*) ICON_FA_ARROW_DOWN "")) {
                changed |= true;

                uint16_t nSwap = n - 1;
                if (nSwap >= 0 && nSwap < arrangementPtr->parts.size()) {
                    std::swap(arrangementPtr->parts.at(n), arrangementPtr->parts.at(nSwap));
                    appState.selectedPart = nSwap;
                }
            }

            ImGui::PopStyleVar();

            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void WindowInspector::Update() {
    GET_APP_STATE;
    GET_ANIMATABLE;

    static InspectorLevel lastInspectorLevel{ inspectorLevel };

    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_MenuBar |
        (
            (inspectorLevel == InspectorLevel_Arrangement || inspectorLevel == InspectorLevel_Arrangement_Im) ?
            ImGuiWindowFlags_NoScrollbar :
            0
        )
    );

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Level")) {
            if (ImGui::MenuItem("Animation", nullptr, inspectorLevel == InspectorLevel_Animation))
                inspectorLevel = InspectorLevel_Animation;
            if (ImGui::MenuItem("Key", nullptr, inspectorLevel == InspectorLevel_Key))
                inspectorLevel = InspectorLevel_Key;
            if (ImGui::MenuItem("Arrangement (Immediate)", nullptr, inspectorLevel == InspectorLevel_Arrangement_Im)) {
                inspectorLevel = InspectorLevel_Arrangement_Im;

                appState.focusOnSelectedPart = true;
                appState.selectedPart = -1;
            }
            if (ImGui::MenuItem("Arrangement (Outside Anim)", nullptr, inspectorLevel == InspectorLevel_Arrangement)) {
                inspectorLevel = InspectorLevel_Arrangement;

                appState.arrangementMode = true;
                appState.focusOnSelectedPart = true;

                appState.controlKey.arrangementIndex = globalAnimatable->getCurrentKey()->arrangementIndex;
                appState.selectedPart = -1;

                appState.playerState.ToggleAnimating(false);
                globalAnimatable->SubmitAnimationKeyPtr(&appState.controlKey);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Enable real canvas offset (512, 512)", nullptr, this->realPosition)) {
                this->realPosition = !this->realPosition;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    bool lastWasArrangement = lastInspectorLevel == InspectorLevel_Arrangement &&
        inspectorLevel != InspectorLevel_Arrangement;
    bool lastWasArrangementIm = lastInspectorLevel == InspectorLevel_Arrangement_Im &&
        inspectorLevel != InspectorLevel_Arrangement_Im;

    if (lastWasArrangement || lastWasArrangementIm) {
        if (inspectorLevel != InspectorLevel_Arrangement && inspectorLevel != InspectorLevel_Arrangement_Im)
            appState.focusOnSelectedPart = false;
    }
        
    if (lastWasArrangement)
        globalAnimatable->setAnimation(appState.selectedAnimation);

    lastInspectorLevel = inspectorLevel;

    appState.arrangementMode = inspectorLevel == InspectorLevel_Arrangement;

    switch (inspectorLevel) {
        case InspectorLevel_Animation:
            this->Level_Animation();
            break;
    
        case InspectorLevel_Key:
            this->Level_Key();
            break;

        case InspectorLevel_Arrangement:
            this->Level_Arrangement();
            break;

        case InspectorLevel_Arrangement_Im:
            this->Level_Arrangement();
            break;

        default:
            ImGui::Text("Inspector level not implemented.");
            break;
    }

    ImGui::End();
}