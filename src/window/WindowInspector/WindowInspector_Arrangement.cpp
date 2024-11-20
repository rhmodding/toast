#include "../WindowInspector.hpp"

// #include "../../ConfigManager.hpp"
#include "../../SessionManager.hpp"

#include "../../anim/Animatable.hpp"

#include "../../command/CommandModifyArrangement.hpp"
#include "../../command/CommandModifyArrangementPart.hpp"
#include "../../command/CommandInsertArrangementPart.hpp"
#include "../../command/CommandDeleteArrangementPart.hpp"
#include "../../command/CommandMoveArrangementPart.hpp"

#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../anim/CellanimHelpers.hpp"

#include "../../font/FontAwesome.h"

#include "../../AppState.hpp"

void WindowInspector::Level_Arrangement() {
    GET_APP_STATE;
    GET_ANIMATABLE;
    GET_SESSION_MANAGER;

    static std::vector<RvlCellAnim::ArrangementPart> copyParts;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

    ImGui::BeginChild("ArrangementOverview", { 0.f, (ImGui::GetWindowSize().y / 2.f) }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
    ImGui::PopStyleVar();
    {
        this->DrawPreview(&globalAnimatable);

        ImGui::SameLine();

        ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
        {
            //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

            ImGui::PushFont(appState.fonts.large);
            ImGui::TextWrapped("Arrangement no.");
            ImGui::PopFont();

            // Arrangement Input
            {
                auto originalKey = *globalAnimatable.getCurrentKey();
                auto newKey = *globalAnimatable.getCurrentKey();

                static uint16_t oldArrangement { 0 };
                uint16_t newArrangement = globalAnimatable.getCurrentKey()->arrangementIndex + 1;

                ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15.f);
                if (ImGui::InputScalar(
                    "##ArrangementInput",
                    ImGuiDataType_U16,
                    &newArrangement,
                    nullptr, nullptr,
                    "%u"
                )) {
                    globalAnimatable.getCurrentKey()->arrangementIndex =
                        std::min<uint16_t>(newArrangement - 1, globalAnimatable.cellanim->arrangements.size() - 1);

                    appState.correctSelectedParts();
                }

                if (ImGui::IsItemActivated())
                    oldArrangement = originalKey.arrangementIndex;

                if (ImGui::IsItemDeactivated() && !appState.getArrangementMode()) {
                    originalKey.arrangementIndex = oldArrangement;
                    newKey.arrangementIndex =
                        std::min<uint16_t>(newArrangement - 1, globalAnimatable.cellanim->arrangements.size() - 1);
                }

                // Start +- Buttons

                const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

                ImGui::PushButtonRepeat(true);

                ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
                if (
                    ImGui::Button("-##ArrangementInput_dec", buttonSize) &&
                    globalAnimatable.getCurrentKey()->arrangementIndex > 0
                ) {
                    if (!appState.getArrangementMode())
                        newKey.arrangementIndex--;
                    else
                        globalAnimatable.getCurrentKey()->arrangementIndex--;

                    appState.correctSelectedParts();
                }
                ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
                if (
                    ImGui::Button("+##ArrangementInput_inc", buttonSize) &&
                    (globalAnimatable.getCurrentKey()->arrangementIndex + 1) < globalAnimatable.cellanim->arrangements.size()
                ) {
                    if (!appState.getArrangementMode())
                        newKey.arrangementIndex++;
                    else
                        globalAnimatable.getCurrentKey()->arrangementIndex++;

                    appState.correctSelectedParts();
                }

                ImGui::PopButtonRepeat();

                {
                    bool arrangementUnique = CellanimHelpers::getArrangementUnique(globalAnimatable.getCurrentKey()->arrangementIndex);
                    ImGui::BeginDisabled(arrangementUnique);

                    if (ImGui::Button("Make arrangement unique (duplicate)"))
                        newKey.arrangementIndex =
                            CellanimHelpers::DuplicateArrangement(originalKey.arrangementIndex);

                    ImGui::EndDisabled();

                    if (
                        arrangementUnique &&
                        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
                        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)
                    )
                        ImGui::OpenPopup("DuplicateAnywayPopup");

                    if (ImGui::BeginPopup("DuplicateAnywayPopup")) {
                        ImGui::Text("Would you like to duplicate this\narrangement anyway?");
                        ImGui::Separator();

                        if (ImGui::Selectable("Ok"))
                            newKey.arrangementIndex =
                                CellanimHelpers::DuplicateArrangement(originalKey.arrangementIndex);
                        ImGui::Selectable("Nevermind");

                        ImGui::EndPopup();
                    }

                    if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("This arrangement is only used once.\nYou can right-click to duplicate anyway.");
                }

                if (newKey != originalKey) {
                    *globalAnimatable.getCurrentKey() = originalKey;

                    SessionManager::getInstance().getCurrentSession()->executeCommand(
                    std::make_shared<CommandModifyAnimationKey>(
                        sessionManager.getCurrentSession()->currentCellanim,
                        appState.globalAnimatable.getCurrentAnimationIndex(),
                        appState.globalAnimatable.getCurrentKeyIndex(),
                        newKey
                    ));
                }
            }

            //ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable.getCurrentArrangement();

        RvlCellAnim::ArrangementPart* partPtr = appState.singlePartSelected() ? &arrangementPtr->parts.at(appState.selectedParts[0].index) : nullptr;

        if (partPtr) {
            RvlCellAnim::ArrangementPart newPart = *partPtr;
            RvlCellAnim::ArrangementPart originalPart = *partPtr;

            ImGui::BeginDisabled(partPtr->editorLocked);

            ImGui::SeparatorText((char*)ICON_FA_PENCIL " Part Name (editor)");
            {
                ImGui::InputText("Name", partPtr->editorName, 32);
            }

            ImGui::SeparatorText((char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Part Transform");
            {
                // Position XY
                {
                    static int oldPosition[2] { 0, 0 }; // Does not apply realPosition offset
                    int positionValues[2] {
                        partPtr->transform.positionX - (this->realPosition ? 0 : CANVAS_ORIGIN),
                        partPtr->transform.positionY - (this->realPosition ? 0 : CANVAS_ORIGIN)
                    };

                    if (ImGui::DragInt2(
                        "Position XY##World",
                        positionValues, 1.f,
                        std::numeric_limits<int16_t>::min(),
                        std::numeric_limits<int16_t>::max()
                    )) {
                        partPtr->transform.positionX = static_cast<int16_t>(
                            positionValues[0] + (this->realPosition ? 0 : CANVAS_ORIGIN)
                        );
                        partPtr->transform.positionY = static_cast<int16_t>(
                            positionValues[1] + (this->realPosition ? 0 : CANVAS_ORIGIN)
                        );
                    }

                    if (ImGui::IsItemActivated()) {
                        oldPosition[0] = originalPart.transform.positionX;
                        oldPosition[1] = originalPart.transform.positionY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        originalPart.transform.positionX = oldPosition[0];
                        originalPart.transform.positionY = oldPosition[1];

                        newPart.transform.positionX = static_cast<int16_t>(
                            positionValues[0] + (this->realPosition ? 0 : CANVAS_ORIGIN)
                        );
                        newPart.transform.positionY = static_cast<int16_t>(
                            positionValues[1] + (this->realPosition ? 0 : CANVAS_ORIGIN)
                        );
                    }
                }

                // Scale XY
                {
                    static float oldScale[2]{ 0.f, 0.f };
                    float scaleValues[2] { partPtr->transform.scaleX, partPtr->transform.scaleY };

                    if (ImGui::DragFloat2("Scale XY##World", scaleValues, .01f)) {
                        partPtr->transform.scaleX = scaleValues[0];
                        partPtr->transform.scaleY = scaleValues[1];
                    }

                    if (ImGui::IsItemActivated()) {
                        oldScale[0] = originalPart.transform.scaleX;
                        oldScale[1] = originalPart.transform.scaleY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        originalPart.transform.scaleX = oldScale[0];
                        originalPart.transform.scaleY = oldScale[1];

                        newPart.transform.scaleX = scaleValues[0];
                        newPart.transform.scaleY = scaleValues[1];
                    }
                }
            }

            // Angle Z slider
            {
                static float oldAngle { 0.f };
                float newAngle = partPtr->transform.angle;

                if (ImGui::SliderFloat("Angle Z", &newAngle, -360.f, 360.f, "%.1f deg"))
                    partPtr->transform.angle = newAngle;

                if (ImGui::IsItemActivated())
                    oldAngle = originalPart.transform.angle;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.transform.angle = oldAngle;
                    newPart.transform.angle = newAngle;
                }
            }

            ImGui::Checkbox("Flip X", &newPart.flipX);
            ImGui::Checkbox("Flip Y", &newPart.flipY);

            ImGui::SeparatorText((char*)ICON_FA_IMAGE " Rendering");

            // Opacity slider
            {
                static const uint8_t min { 0 };
                static const uint8_t max { 0xFF };

                static uint8_t oldOpacity { 0 };
                uint8_t newOpacity = partPtr->opacity;

                if (ImGui::SliderScalar("Opacity", ImGuiDataType_U8, &newOpacity, &min, &max, "%u"))
                    partPtr->opacity = newOpacity;

                if (ImGui::IsItemActivated())
                    oldOpacity = originalPart.opacity;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.opacity = oldOpacity;
                    newPart.opacity = newOpacity;
                }
            }

            ImGui::SeparatorText((char*)ICON_FA_BORDER_TOP_LEFT " Region");
            {
                // Position XY
                {
                    static int oldPosition[2] { 0, 0 };
                    int positionValues[2] {
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
                        originalPart.regionX = static_cast<uint16_t>(oldPosition[0]);
                        originalPart.regionY = static_cast<uint16_t>(oldPosition[1]);

                        newPart.regionX = static_cast<uint16_t>(positionValues[0]);
                        newPart.regionY = static_cast<uint16_t>(positionValues[1]);
                    }
                }

                // Size WH
                {
                    static int oldSize[2] { 0, 0 };
                    int sizeValues[2] {
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
                        originalPart.regionW = static_cast<uint16_t>(oldSize[0]);
                        originalPart.regionH = static_cast<uint16_t>(oldSize[1]);

                        newPart.regionW = static_cast<uint16_t>(sizeValues[0]);
                        newPart.regionH = static_cast<uint16_t>(sizeValues[1]);
                    }
                }
            }

            if (newPart != originalPart) {
                *partPtr = originalPart;

                SessionManager::getInstance().getCurrentSession()->executeCommand(
                    std::make_shared<CommandModifyArrangementPart>(newPart)
                );
            }

            ImGui::EndDisabled();
        }
        else {
            if (appState.multiplePartsSelected())
                ImGui::TextWrapped("Multiple parts selected -- inspector unavaliable.");
            else
                ImGui::TextWrapped("No part selected.");
        }
    }
    ImGui::EndChild();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

    ImGui::BeginChild("ArrangementParts", { 0.f, 0.f }, ImGuiChildFlags_Border);
    ImGui::PopStyleVar();
    {
        ImGui::SeparatorText((char*)ICON_FA_IMAGE " Parts");

        //ImGui::Text("Selected size : %u, next : %i", (unsigned)appState.selectedParts.size(), appState.spSelectionOrder);

        RvlCellAnim::Arrangement* arrangementPtr =
            &globalAnimatable.cellanim->arrangements.at(globalAnimatable.getCurrentKey()->arrangementIndex);

        ImGui::BeginChild("PartList", { 0.f, 0.f }, ImGuiChildFlags_None);

        const ImVec2 listChildSize = ImGui::GetContentRegionAvail();

        ImGuiMultiSelectFlags msFlags =
            ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
            ImGuiMultiSelectFlags_BoxSelect1d;
        ImGuiMultiSelectIO* msIo = ImGui::BeginMultiSelect(msFlags, appState.selectedParts.size(), arrangementPtr->parts.size());

        appState.processMultiSelectRequests(msIo);

        bool wantDeleteSelected = (
            ImGui::Shortcut(ImGuiKey_Backspace | ImGuiKey_ModCtrl, ImGuiInputFlags_Repeat) ||
            ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat)
        ) && appState.anyPartsSelected();

        int itemCurrentIndexToFocus = wantDeleteSelected ?
            appState.getNextPartIndexAfterDeletion(msIo, arrangementPtr->parts.size()) :
            -1;

        int selDeleteSingle = -1; // part index to delete
        int insertNewPart = -1; // new part index

        for (int n = arrangementPtr->parts.size() - 1; n >= 0; n--) {
            ImGui::PushID(n);

            RvlCellAnim::ArrangementPart& part = arrangementPtr->parts.at(n);

            char selectableLabel[128];
            if (part.editorName[0] == '\0')
                snprintf(
                    selectableLabel, sizeof(selectableLabel),
                    "Part no. %u", n+1
                );
            else
                snprintf(
                    selectableLabel, sizeof(selectableLabel),
                    "Part no. %u (%s)", n+1, part.editorName
                );

            const bool isPartSelected = appState.isPartSelected(n);

            ImGui::SetNextItemAllowOverlap();

            ImGui::SetNextItemSelectionUserData(n);
            ImGui::Selectable("###PartSelectable", isPartSelected, 0);
            if (itemCurrentIndexToFocus == n)
                ImGui::SetKeyboardFocusHere(-1);

            //bool deletePart { false };
            if (ImGui::BeginPopupContextItem()) {
                char label[128];

                if (isPartSelected) {
                    snprintf(
                        label, sizeof(label),
                        "%zu selected parts",
                        appState.selectedParts.size()
                    );
                }
                else
                    strcpy(label, selectableLabel);

                ImGui::TextUnformatted(label);

                // 0 = disabled, 1 = enabled, anything else = mixed
                int visible = -1;
                int locked = -1;

                if (isPartSelected) {
                    for (const auto& [index, _] : appState.selectedParts) {
                        const auto& part = arrangementPtr->parts.at(index);

                        if (visible == -1)
                            visible = part.editorVisible;
                        else if (visible != part.editorVisible)
                            visible = 2; // Set mixed

                        if (locked == -1)
                            locked = part.editorLocked;
                        else if (locked != part.editorLocked)
                            locked = 2; // Set mixed
                    }
                }
                else {
                    visible = part.editorVisible;
                    locked = part.editorLocked;
                }

                ImGui::BulletText("Visible: %s", (visible == 0) ? "no" : (visible == 1) ? "yes" : "mixed");
                ImGui::BulletText("Locked: %s", (locked == 0) ? "no" : (locked == 1) ? "yes" : "mixed");
                ImGui::Separator();

                ImGui::BeginDisabled(locked);

                if (ImGui::Selectable("Insert new part above"))
                    insertNewPart = n + 1;
                if (ImGui::Selectable("Insert new part below"))
                    insertNewPart = n;

                ImGui::Separator();

                snprintf(
                    label, sizeof(label),
                    "Paste %zu part%s..",
                    copyParts.size(), copyParts.size() == 1 ? "" : "s"
                );

                if (ImGui::BeginMenu(label, !copyParts.empty())) {
                    if (ImGui::MenuItem("..above")) {
                        auto newArrangement = *appState.globalAnimatable.getCurrentArrangement();
                        newArrangement.parts.insert(
                            newArrangement.parts.begin() + n + 1,
                            copyParts.begin(),
                            copyParts.end()
                        );

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyArrangement>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                            newArrangement
                        ));

                        appState.clearSelectedParts();
                        for (unsigned i = 0; i < copyParts.size(); i++)
                            appState.setPartSelected(n + 1 + i, true);
                        //appState.setPartSelected(n + 1, true);
                    }
                    if (ImGui::MenuItem("..below")) {
                        auto newArrangement = *appState.globalAnimatable.getCurrentArrangement();
                        newArrangement.parts.insert(
                            newArrangement.parts.begin() + n,
                            copyParts.begin(),
                            copyParts.end()
                        );

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyArrangement>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                            newArrangement
                        ));

                        appState.clearSelectedParts();
                        for (unsigned i = 0; i < copyParts.size(); i++)
                            appState.setPartSelected(n + i, true);
                        //appState.setPartSelected(n, true);
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("..here (replace)")) {
                        auto newArrangement = *appState.globalAnimatable.getCurrentArrangement();

                        newArrangement.parts.erase(newArrangement.parts.begin() + n);
                        newArrangement.parts.insert(
                            newArrangement.parts.begin() + n,
                            copyParts.begin(),
                            copyParts.end()
                        );

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyArrangement>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                            newArrangement
                        ));

                        appState.clearSelectedParts();
                        for (unsigned i = 0; i < copyParts.size(); i++)
                            appState.setPartSelected(n + i, true);
                        //appState.setPartSelected(n, true);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Paste single part (special)..", copyParts.size() == 1)) {
                    if (ImGui::MenuItem("..transform")) {
                        RvlCellAnim::ArrangementPart newPart = part;

                        newPart.transform = copyParts[0].transform;

                        newPart.flipX = copyParts[0].flipX;
                        newPart.flipY = copyParts[0].flipY;

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyArrangementPart>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                            n,
                            newPart
                        ));

                        appState.clearSelectedParts();
                        appState.setPartSelected(n, true);
                    }

                    if (ImGui::MenuItem("..opacity")) {
                        RvlCellAnim::ArrangementPart newPart = part;
                        newPart.opacity = copyParts[0].opacity;

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyArrangementPart>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                            n,
                            newPart
                        ));

                        appState.clearSelectedParts();
                        appState.setPartSelected(n, true);
                    }

                    if (ImGui::MenuItem("..region")) {
                        RvlCellAnim::ArrangementPart newPart = part;

                        newPart.regionX = copyParts[0].regionX;
                        newPart.regionY = copyParts[0].regionY;

                        newPart.regionW = copyParts[0].regionW;
                        newPart.regionH = copyParts[0].regionH;

                        sessionManager.getCurrentSession()->executeCommand(
                        std::make_shared<CommandModifyArrangementPart>(
                            sessionManager.getCurrentSession()->currentCellanim,
                            appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                            n,
                            newPart
                        ));

                        appState.clearSelectedParts();
                        appState.setPartSelected(n, true);
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (isPartSelected) {
                    snprintf(
                        label, sizeof(label),
                        "Copy %zu part%s",
                        appState.selectedParts.size(),
                        appState.selectedParts.size() == 1 ? "" : "s"
                    );
                }
                else
                    strcpy(label, "Copy part");

                if (ImGui::Selectable(label)) {
                    if (isPartSelected) {
                        copyParts.resize(appState.selectedParts.size());
                        for (unsigned i = 0; i < appState.selectedParts.size(); i++) {
                            copyParts[i] = arrangementPtr->parts.at(
                                appState.selectedParts[i].index
                            );
                        }
                    }
                    else {
                        copyParts.resize(1);
                        copyParts[0] = part;
                    }
                }

                ImGui::Separator();

                if (isPartSelected) {
                    snprintf(
                        label, sizeof(label),
                        "Delete %zu part%s",
                        appState.selectedParts.size(),
                        appState.selectedParts.size() == 1 ? "" : "s"
                    );
                }
                else
                    strcpy(label, "Delete part");

                if (ImGui::Selectable(label)) {
                    if (isPartSelected) {
                        wantDeleteSelected = true;
                        itemCurrentIndexToFocus =
                            appState.getNextPartIndexAfterDeletion(msIo, arrangementPtr->parts.size());
                    }
                    else
                        selDeleteSingle = n;
                }

                ImGui::EndDisabled();

                ImGui::EndPopup();
            }

            auto partToggle = [](const char* label, bool toggled) -> bool {
                ImGuiWindow* window = ImGui::GetCurrentWindow();

                const ImGuiID id = window->GetID(label);
                const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

                const ImGuiStyle& style = ImGui::GetStyle();

                ImVec2 pos = window->DC.CursorPos;

                if (0.f < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
                    pos.y += window->DC.CurrLineTextBaseOffset;

                ImVec2 size = ImGui::CalcItemSize(
                    { 0.f, 0.f },
                    labelSize.x,
                    labelSize.y
                );

                const ImRect bb(pos, { pos.x + size.x, pos.y + size.y });

                ImGui::ItemSize(size, 0.f);
                if (!ImGui::ItemAdd(bb, id))
                    return false;

                bool hovered { false }, held { false };
                bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

                uint32_t textColor { 0xFFFFFFFF };

                ImVec4 color = toggled ?
                    ImVec4(1.f, 0.f, 0.f, 1.f) :
                    style.Colors[ImGuiCol_Text];
                color.w *= style.Alpha * (hovered ? 1.f : .5f);

                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertFloat4ToU32(color));
                ImGui::RenderTextClipped(
                    bb.Min,
                    bb.Max,
                    label, nullptr,
                    &labelSize,
                    { .5f, .5f },

                    &bb
                );
                ImGui::PopStyleColor();

                return pressed;
            };

            ImGui::SameLine(4.f, 0.f);
            if (partToggle((char*)ICON_FA_EYE "##Visible", !part.editorVisible)) {
                if (!isPartSelected)
                    part.editorVisible ^= true;
                else {
                    for (const auto& [index, _] : appState.selectedParts)
                        arrangementPtr->parts.at(index).editorVisible ^= true;
                }
            }

            ImGui::SameLine();
            if (partToggle((char*)ICON_FA_LOCK "##Locked", part.editorLocked)) {
                if (!isPartSelected)
                    part.editorLocked ^= true;
                else {
                    for (const auto& [index, _] : appState.selectedParts)
                        arrangementPtr->parts.at(index).editorLocked ^= true;
                }
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(part.editorLocked);
            ImGui::TextUnformatted(selectableLabel);
            ImGui::EndDisabled();

            const ImGuiStyle& style = ImGui::GetStyle();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.f, style.ItemSpacing.y });

            float firstButtonWidth = ImGui::CalcTextSize((char*) ICON_FA_ARROW_DOWN "").x + (style.FramePadding.x * 2);
            float basePositionX = listChildSize.x - (style.FramePadding.x * 2) - style.ScrollbarSize;

            ImGui::SameLine();
            ImGui::SetCursorPosX(basePositionX - firstButtonWidth - style.ItemSpacing.x);

            ImGui::BeginDisabled(part.editorLocked);

            ImGui::BeginDisabled(n == arrangementPtr->parts.size() - 1);
            if (ImGui::SmallButton((char*) ICON_FA_ARROW_UP "")) {
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandMoveArrangementPart>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                    n,
                    false
                ));
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::SetCursorPosX(basePositionX);

            ImGui::BeginDisabled(n == 0);
            if (ImGui::SmallButton((char*) ICON_FA_ARROW_DOWN "")) {
                sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandMoveArrangementPart>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                    n,
                    true
                ));
            }
            ImGui::EndDisabled();

            ImGui::EndDisabled();

            ImGui::PopStyleVar();

            ImGui::PopID();
        }

        msIo = ImGui::EndMultiSelect();
        appState.processMultiSelectRequests(msIo);

        ImGui::EndChild();

        if (wantDeleteSelected) {
            auto newArrangement = *arrangementPtr;
            appState.deleteSelectedParts(msIo, newArrangement.parts, itemCurrentIndexToFocus);

            sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandModifyArrangement>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    globalAnimatable.getCurrentKey()->arrangementIndex,
                    newArrangement
                )
            );
        }

        if (selDeleteSingle >= 0) {
            sessionManager.getCurrentSession()->executeCommand(
                std::make_shared<CommandDeleteArrangementPart>(
                    sessionManager.getCurrentSession()->currentCellanim,
                    globalAnimatable.getCurrentKey()->arrangementIndex,
                    selDeleteSingle
                )
            );
        }

        if (arrangementPtr->parts.size() == 0)
        if (ImGui::Selectable("Click here to create a new part."))
            insertNewPart = 0;

        if (insertNewPart >= 0) {
            sessionManager.getCurrentSession()->executeCommand(
            std::make_shared<CommandInsertArrangementPart>(
                sessionManager.getCurrentSession()->currentCellanim,
                appState.globalAnimatable.getCurrentKey()->arrangementIndex,
                insertNewPart,
                RvlCellAnim::ArrangementPart {}
            ));

            appState.clearSelectedParts();
            appState.setPartSelected(insertNewPart, true);
        }
    }
    ImGui::EndChild();
}
