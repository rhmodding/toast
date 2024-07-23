#include "../WindowInspector.hpp"

#include "../../font/FontAwesome.h"

#include "../../ConfigManager.hpp"
#include "../../SessionManager.hpp"

#include "../../command/CommandModifyArrangementPart.hpp"
#include "../../command/CommandInsertArrangementPart.hpp"
#include "../../command/CommandDeleteArrangementPart.hpp"
#include "../../command/CommandMoveArrangementPart.hpp"

#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../AppState.hpp"
#include "../../anim/Animatable.hpp"

const uint8_t uint8_one = 1;
const uint16_t uint16_one = 1;
const uint32_t uint32_one = 1;

void WindowInspector::Level_Arrangement() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    static RvlCellAnim::ArrangementPart copyPart;
    static bool allowPastePart{ false };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

    ImGui::BeginChild("ArrangementOverview", { 0, (ImGui::GetWindowSize().y / 2.f) }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
    ImGui::PopStyleVar();
    {
        this->DrawPreview(globalAnimatable);

        ImGui::SameLine();

        ImGui::BeginChild("LevelHeader", { 0, 0 }, ImGuiChildFlags_AutoResizeY);
        {
            //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

            ImGui::PushFont(appState.fonts.large);
            ImGui::TextWrapped("Arrangement no.");
            ImGui::PopFont();

            // Arrangement Input
            {
                auto originalKey = *globalAnimatable->getCurrentKey();
                auto newKey = *globalAnimatable->getCurrentKey();

                static uint16_t oldArrangement{ 0 };
                uint16_t newArrangement = globalAnimatable->getCurrentKey()->arrangementIndex + 1;

                ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                if (ImGui::InputScalar(
                    "##ArrangementInput",
                    ImGuiDataType_U16,
                    &newArrangement,
                    nullptr, nullptr,
                    "%u"
                )) {
                    globalAnimatable->getCurrentKey()->arrangementIndex =
                        std::clamp<uint16_t>(newArrangement - 1, 0, globalAnimatable->cellanim->arrangements.size());

                    appState.correctSelectedPart();
                }

                if (ImGui::IsItemActivated())
                    oldArrangement = originalKey.arrangementIndex;

                if (ImGui::IsItemDeactivated() && !appState.getArrangementMode()) {
                    changed = true;

                    originalKey.arrangementIndex = oldArrangement;
                    newKey.arrangementIndex =
                        std::clamp<uint16_t>(newArrangement - 1, 0, globalAnimatable->cellanim->arrangements.size());
                }

                // Start +- Buttons

                const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

                ImGui::PushButtonRepeat(true);

                ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
                if (
                    ImGui::Button("-##ArrangementInput_dec", buttonSize) &&
                    globalAnimatable->getCurrentKey()->arrangementIndex > 0
                ) {
                    if (!appState.getArrangementMode())
                        newKey.arrangementIndex--;
                    else
                        globalAnimatable->getCurrentKey()->arrangementIndex--;

                    appState.correctSelectedPart();
                }
                ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
                if (
                    ImGui::Button("+##ArrangementInput_inc", buttonSize) &&
                    (globalAnimatable->getCurrentKey()->arrangementIndex + 1) < globalAnimatable->cellanim->arrangements.size()
                ) {
                    if (!appState.getArrangementMode())
                        newKey.arrangementIndex++;
                    else
                        globalAnimatable->getCurrentKey()->arrangementIndex++;

                    appState.correctSelectedPart();
                }

                ImGui::PopButtonRepeat();

                if (newKey != originalKey) {
                    *globalAnimatable->getCurrentKey() = originalKey;

                    SessionManager::getInstance().getCurrentSession()->executeCommand(
                    std::make_shared<CommandModifyAnimationKey>(
                    CommandModifyAnimationKey(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentAnimationIndex(),
                        appState.globalAnimatable->getCurrentKeyIndex(),
                        newKey
                    )));
                }
            }
            
            //ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

        RvlCellAnim::ArrangementPart* partPtr = appState.selectedPart >= 0 ? &arrangementPtr->parts.at(appState.selectedPart) : nullptr;

        if (partPtr) {
            RvlCellAnim::ArrangementPart newPart = *partPtr;
            RvlCellAnim::ArrangementPart originalPart = *partPtr;

            ImGui::SeparatorText((char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Part Transform");
            {
                // Position XY
                {
                    static int oldPosition[2]{ 0, 0 }; // Does not apply realPosition offset
                    int positionValues[2] = {
                        partPtr->positionX - (this->realPosition ? 0 : 512),
                        partPtr->positionY - (this->realPosition ? 0 : 512)
                    };
                    if (ImGui::DragInt2("Position XY##World", positionValues, 1.f, INT16_MIN, INT16_MAX)) {
                        partPtr->positionX = static_cast<int16_t>(
                            positionValues[0] + (this->realPosition ? 0 : 512)
                        );
                        partPtr->positionY = static_cast<int16_t>(
                            positionValues[1] + (this->realPosition ? 0 : 512)
                        );
                    }

                    if (ImGui::IsItemActivated()) {
                        oldPosition[0] = originalPart.positionX;
                        oldPosition[1] = originalPart.positionY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        changed = true;

                        originalPart.positionX = oldPosition[0];
                        originalPart.positionY = oldPosition[1];

                        newPart.positionX = static_cast<int16_t>(
                            positionValues[0] + (this->realPosition ? 0 : 512)
                        );
                        newPart.positionY = static_cast<int16_t>(
                            positionValues[1] + (this->realPosition ? 0 : 512)
                        );
                    }
                }

                // Scale XY
                {
                    static float oldScale[2]{ 0.f, 0.f };
                    float scaleValues[2] = { partPtr->scaleX, partPtr->scaleY };
                    if (ImGui::DragFloat2("Scale XY##World", scaleValues, 0.01f)) {
                        partPtr->scaleX = scaleValues[0];
                        partPtr->scaleY = scaleValues[1];
                    }

                    if (ImGui::IsItemActivated()) {
                        oldScale[0] = originalPart.scaleX;
                        oldScale[1] = originalPart.scaleY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        changed = true;

                        originalPart.scaleX = oldScale[0];
                        originalPart.scaleY = oldScale[1];

                        newPart.scaleX = scaleValues[0];
                        newPart.scaleY = scaleValues[1];
                    }
                }
            }

            // Angle Z slider
            {
                static float oldAngle{ 0.f };
                float newAngle = partPtr->angle;
                if (ImGui::SliderFloat("Angle Z", &newAngle, -360.f, 360.f, "%.1f deg"))
                    partPtr->angle = newAngle;

                if (ImGui::IsItemActivated())
                    oldAngle = originalPart.angle;

                if (ImGui::IsItemDeactivated()) {
                    changed = true;

                    originalPart.angle = oldAngle;
                    newPart.angle = newAngle;
                }
            }

            changed |= ImGui::Checkbox("Flip X", &newPart.flipX);
            changed |= ImGui::Checkbox("Flip Y", &newPart.flipY);

            ImGui::SeparatorText((char*)ICON_FA_IMAGE " Rendering");

            // Opacity slider
            {
                static const uint8_t min{ 0 };
                static const uint8_t max{ 0xFF };

                static uint8_t oldOpacity{ 0 };
                uint8_t newOpacity = partPtr->opacity;
                if (ImGui::SliderScalar("Opacity", ImGuiDataType_U8, &newOpacity, &min, &max, "%u"))
                    partPtr->opacity = newOpacity;

                if (ImGui::IsItemActivated())
                    oldOpacity = originalPart.opacity;

                if (ImGui::IsItemDeactivated()) {
                    changed = true;

                    originalPart.opacity = oldOpacity;
                    newPart.opacity = newOpacity;
                }
            }

            ImGui::SeparatorText((char*)ICON_FA_BORDER_TOP_LEFT " Region");
            {
                // Position XY
                {
                    static int oldPosition[2]{ 0, 0 };
                    int positionValues[2] = {
                        partPtr->regionX,
                        partPtr->regionY
                    };
                    if (ImGui::DragInt2("Position XY##Region", positionValues, 1.f)) {
                        partPtr->regionX = static_cast<uint16_t>(positionValues[0]);
                        partPtr->regionY = static_cast<uint16_t>(positionValues[1]);
                    }

                    if (ImGui::IsItemActivated()) {
                        oldPosition[0] = originalPart.regionX;
                        oldPosition[1] = originalPart.regionY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        changed = true;

                        originalPart.regionX = static_cast<uint16_t>(oldPosition[0]);
                        originalPart.regionY = static_cast<uint16_t>(oldPosition[1]);

                        newPart.regionX = static_cast<uint16_t>(positionValues[0]);
                        newPart.regionY = static_cast<uint16_t>(positionValues[1]);
                    }
                }

                // Size WH
                {
                    static int oldSize[2]{ 0, 0 };
                    int sizeValues[2] = {
                        newPart.regionW,
                        newPart.regionH
                    };
                    if (ImGui::DragInt2("Size WH##Region", sizeValues, 1.f)) {
                        partPtr->regionW = static_cast<uint16_t>(sizeValues[0]);
                        partPtr->regionH = static_cast<uint16_t>(sizeValues[1]);
                    }

                    if (ImGui::IsItemActivated()) {
                        oldSize[0] = originalPart.regionW;
                        oldSize[1] = originalPart.regionH;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        changed = true;

                        originalPart.regionW = static_cast<uint16_t>(oldSize[0]);
                        originalPart.regionH = static_cast<uint16_t>(oldSize[1]);

                        newPart.regionW = static_cast<uint16_t>(sizeValues[0]);
                        newPart.regionH = static_cast<uint16_t>(sizeValues[1]);
                    }
                }
            }

            if (ConfigManager::getInstance().getConfig().showUnknownValues) {
                ImGui::SeparatorText((char*)ICON_FA_CIRCLE_QUESTION " Unknown value (byteswapped)..");

                if (ImGui::CollapsingHeader("..as Uint32", ImGuiTreeNodeFlags_None)) {
                    changed |= ImGui::InputScalar(" A##Unk_Uint32", ImGuiDataType_U32, &newPart.unknown.u32, &uint32_one, nullptr, "%u");
                }
                ImGui::Separator();
                if (ImGui::CollapsingHeader("..as Uint16", ImGuiTreeNodeFlags_None)) {
                    changed |= ImGui::InputScalar(" A##Unk_Uint16", ImGuiDataType_U16, &newPart.unknown.u16[0], &uint16_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" B##Unk_Uint16", ImGuiDataType_U16, &newPart.unknown.u16[1], &uint16_one, nullptr, "%u");
                }
                ImGui::Separator();
                if (ImGui::CollapsingHeader("..as Uint8 (byte)", ImGuiTreeNodeFlags_None)) {
                    changed |= ImGui::InputScalar(" A##Unk_Uint8", ImGuiDataType_U8, &newPart.unknown.u8[0], &uint8_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" B##Unk_Uint8", ImGuiDataType_U8, &newPart.unknown.u8[1], &uint8_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" C##Unk_Uint8", ImGuiDataType_U8, &newPart.unknown.u8[2], &uint8_one, nullptr, "%u");
                    changed |= ImGui::InputScalar(" D##Unk_Uint8", ImGuiDataType_U8, &newPart.unknown.u8[3], &uint8_one, nullptr, "%u");
                }
            }

            if (newPart != originalPart) {
                *partPtr = originalPart;

                SessionManager::getInstance().getCurrentSession()->executeCommand(
                std::make_shared<CommandModifyArrangementPart>(
                    CommandModifyArrangementPart(newPart)
                ));
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
            if (ImGui::Selectable(buffer, appState.selectedPart == n, ImGuiSelectableFlags_SelectOnNav))
                appState.selectedPart = n;

            bool deletePart{ false };
            if (ImGui::BeginPopupContextItem()) {
                ImGui::TextUnformatted(buffer);
                ImGui::Separator();

                if (ImGui::Selectable("Insert new part above")) {
                    sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandInsertArrangementPart>(
                    CommandInsertArrangementPart(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                        n + 1,
                        RvlCellAnim::ArrangementPart()
                    )));

                    changed = true;

                    appState.selectedPart = n + 1;
                }
                if (ImGui::Selectable("Insert new part below")) {
                    sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandInsertArrangementPart>(
                    CommandInsertArrangementPart(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                        n,
                        RvlCellAnim::ArrangementPart()
                    )));

                    changed = true;

                    appState.selectedPart = n;
                };

                ImGui::Separator();

                if (ImGui::BeginMenu("Paste part..", allowPastePart)) {
                    if (ImGui::MenuItem("..above")) {
                        sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandInsertArrangementPart>(
                        CommandInsertArrangementPart(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                            n + 1,
                            copyPart
                        )));

                        changed = true;

                        appState.selectedPart = n + 1;
                    }
                    if (ImGui::MenuItem("..below")) {
                        sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandInsertArrangementPart>(
                        CommandInsertArrangementPart(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                            n,
                            copyPart
                        )));

                        changed = true;

                        appState.selectedPart = n;
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("..here (replace)")) {
                        sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandModifyArrangementPart>(
                        CommandModifyArrangementPart(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                            n,
                            copyPart
                        )));

                        changed = true;

                        appState.selectedPart = n;
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Paste part (special)..", allowPastePart)) {
                    if (ImGui::MenuItem("..transform")) {
                        RvlCellAnim::ArrangementPart newPart = arrangementPtr->parts.at(n);

                        newPart.positionX = copyPart.positionX;
                        newPart.positionY = copyPart.positionY;

                        newPart.scaleX = copyPart.scaleX;
                        newPart.scaleY = copyPart.scaleY;

                        newPart.angle = copyPart.angle;

                        newPart.flipX = copyPart.flipX;
                        newPart.flipY = copyPart.flipY;

                        sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandModifyArrangementPart>(
                        CommandModifyArrangementPart(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                            n,
                            newPart
                        )));

                        changed = true;

                        appState.selectedPart = n;
                    }

                    if (ImGui::MenuItem("..opacity")) {
                        RvlCellAnim::ArrangementPart newPart = arrangementPtr->parts.at(n);
                        newPart.opacity = copyPart.opacity;

                        sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandModifyArrangementPart>(
                        CommandModifyArrangementPart(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                            n,
                            newPart
                        )));

                        changed = true;

                        appState.selectedPart = n;
                    }

                    if (ImGui::MenuItem("..region")) {
                        RvlCellAnim::ArrangementPart newPart = arrangementPtr->parts.at(n);

                        newPart.regionX = copyPart.regionX;
                        newPart.regionY = copyPart.regionY;

                        newPart.regionW = copyPart.regionW;
                        newPart.regionH = copyPart.regionH;

                        sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandModifyArrangementPart>(
                        CommandModifyArrangementPart(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                            n,
                            newPart
                        )));

                        changed = true;

                        appState.selectedPart = n;
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::Selectable("Copy part")) {
                    copyPart = arrangementPtr->parts.at(n);
                    allowPastePart = true;
                }

                ImGui::Separator();
                if (ImGui::Selectable("Delete part", false, ImGuiSelectableFlags_DontClosePopups))
                    deletePart = true;

                ImGui::EndPopup();
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6, ImGui::GetStyle().ItemSpacing.y });

            float firstButtonWidth = ImGui::CalcTextSize((char*) ICON_FA_ARROW_DOWN "").x + (ImGui::GetStyle().FramePadding.x * 2);
            float basePositionX = ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().WindowPadding.x - 10.f;

            ImGui::SameLine();
            ImGui::SetCursorPosX(basePositionX - firstButtonWidth - ImGui::GetStyle().ItemSpacing.x);

            ImGui::BeginDisabled(n == arrangementPtr->parts.size() - 1);
            if (ImGui::SmallButton((char*) ICON_FA_ARROW_UP "")) {
                sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandMoveArrangementPart>(
                CommandMoveArrangementPart(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                    n,
                    false
                )));

                changed = true;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::SetCursorPosX(basePositionX);

            ImGui::BeginDisabled(n == 0);
            if (ImGui::SmallButton((char*) ICON_FA_ARROW_DOWN "")) {
                sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandMoveArrangementPart>(
                CommandMoveArrangementPart(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                    n,
                    true
                )));

                changed = true;
            }
            ImGui::EndDisabled();

            ImGui::PopStyleVar();

            ImGui::PopID();

            if (deletePart) {
                sessionManager.getCurrentSession()->executeCommand(std::make_shared<CommandDeleteArrangementPart>(
                CommandDeleteArrangementPart(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable->getCurrentKey()->arrangementIndex,
                    n
                )));
            }
        }
    
        if (
            (arrangementPtr->parts.size() == 0) &&
            ImGui::Selectable("Click here to create a new part.")
        )
            arrangementPtr->parts.push_back(RvlCellAnim::ArrangementPart());
    }
    ImGui::EndChild();
}