#include "../WindowInspector.hpp"

// #include "../../ConfigManager.hpp"
#include "../../SessionManager.hpp"

#include "../../ThemeManager.hpp"

#include "../../command/CommandModifyArrangement.hpp"
#include "../../command/CommandModifyArrangementPart.hpp"
#include "../../command/CommandInsertArrangementPart.hpp"
#include "../../command/CommandDeleteArrangementPart.hpp"
#include "../../command/CommandMoveArrangementPart.hpp"

#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../font/FontAwesome.h"

#include "../../AppState.hpp"

// Used for the part visibillity & lock buttons.
static bool partToggleButton(const char* label, bool toggled) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImGuiID id = window->GetID(label);
    const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

    const ImGuiStyle& style = ImGui::GetStyle();

    ImVec2 pos = window->DC.CursorPos;

    // Try to vertically align buttons that are smaller/have no padding so that
    // text baseline matches (bit hacky, since it shouldn't be a flag)
    if (0.f < window->DC.CurrLineTextBaseOffset)
        pos.y += window->DC.CurrLineTextBaseOffset;

    const ImVec2 size = ImGui::CalcItemSize(
        { 0.f, 0.f },
        labelSize.x,
        labelSize.y
    );

    const ImRect bb(pos, { pos.x + size.x, pos.y + size.y });

    ImGui::ItemSize(size, 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered { false }, held { false };
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

    ImVec4 color = toggled ?
        ImVec4(1.f, 0.f, 0.f, 1.f) : style.Colors[ImGuiCol_Text];
    color.w *= style.Alpha * (hovered ? 1.f : .5f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertFloat4ToU32(color));
    ImGui::RenderTextClipped(
        bb.Min, bb.Max,
        label, nullptr,
        &labelSize,
        { .5f, .5f },

        &bb
    );
    ImGui::PopStyleColor();

    return pressed;
}

static bool textInputStdString(const char* label, std::string& str) {
    constexpr ImGuiTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    return ImGui::InputText(label, const_cast<char*>(str.c_str()), str.capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) -> int {
        std::string* str = (std::string*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            // Resize string callback
            //
            // If for some reason we refuse the new length (BufTextLen) and/or
            // capacity (BufSize) we need to set them back to what we want.
            IM_ASSERT(data->Buf == str->c_str());

            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }

        return 0;
    }, &str);
}

void WindowInspector::Level_Arrangement() {
    AppState& appState = AppState::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    const bool isCtr = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_CTR;

    static std::vector<CellAnim::ArrangementPart> copyParts;

    const auto& arrangements = sessionManager.getCurrentSession()
        ->getCurrentCellanim().object->arrangements;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

    ImGui::BeginChild("ArrangementOverview", { 0.f, (ImGui::GetWindowSize().y / 2.f) }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
    ImGui::PopStyleVar();
    {
        this->drawPreview();

        ImGui::SameLine();

        ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
        {
            //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

            ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
            ImGui::TextWrapped("Arrangement no.");
            ImGui::PopFont();

            // Arrangement Input
            {
                auto originalKey = playerManager.getKey();
                auto newKey = originalKey;

                static int oldArrangement { 0 };
                int newArrangement = playerManager.getArrangementIndex() + 1;

                ImGui::SetNextItemWidth(152.f);

                if (ImGui::InputInt("##ArrangementInput", &newArrangement)) {
                    if (newArrangement < 1) newArrangement = 1;

                    playerManager.getKey().arrangementIndex = std::min<unsigned>(
                        newArrangement - 1, arrangements.size() - 1
                    );
                    playerManager.correctState();
                }

                if (ImGui::IsItemActivated())
                    oldArrangement = originalKey.arrangementIndex;

                if (ImGui::IsItemDeactivated() && !appState.getArrangementMode()) {
                    originalKey.arrangementIndex = oldArrangement;
                    newKey.arrangementIndex = std::min<unsigned>(
                        newArrangement - 1, arrangements.size() - 1
                    );
                }

                this->duplicateArrangementButton(newKey, originalKey);

                if (newKey != originalKey) {
                    playerManager.getKey() = originalKey;

                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandModifyAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                        playerManager.getAnimationIndex(),
                        playerManager.getKeyIndex(),
                        newKey
                    ));
                }
            }

            //ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        if (appState.singlePartSelected()) {
            auto& part = playerManager.getArrangement().parts.at(appState.selectedParts[0].index);

            CellAnim::ArrangementPart newPart = part;
            CellAnim::ArrangementPart originalPart = part;

            ImGui::BeginDisabled(part.editorLocked);

            ImGui::SeparatorText((const char*)ICON_FA_PENCIL " Name (editor)");

            textInputStdString("Name", part.editorName);

            ImGui::SeparatorText((const char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform");
            {
                // Position XY
                {
                    static int oldPosition[2] { 0, 0 };
                    int positionValues[2] {
                        part.transform.positionX,
                        part.transform.positionY
                    };

                    if (ImGui::DragInt2(
                        "Position XY##World",
                        positionValues, 1.f,
                        CellAnim::TransformValues::MIN_POSITION,
                        CellAnim::TransformValues::MAX_POSITION
                    )) {
                        part.transform.positionX = static_cast<int16_t>(positionValues[0]);
                        part.transform.positionY = static_cast<int16_t>(positionValues[1]);
                    }

                    if (ImGui::IsItemActivated()) {
                        oldPosition[0] = originalPart.transform.positionX;
                        oldPosition[1] = originalPart.transform.positionY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        originalPart.transform.positionX = oldPosition[0];
                        originalPart.transform.positionY = oldPosition[1];

                        newPart.transform.positionX = static_cast<int16_t>(positionValues[0]);
                        newPart.transform.positionY = static_cast<int16_t>(positionValues[1]);
                    }
                }

                // Scale XY
                {
                    static float oldScale[2]{ 0.f, 0.f };
                    float scaleValues[2] { part.transform.scaleX, part.transform.scaleY };

                    if (ImGui::DragFloat2("Scale XY##World", scaleValues, .01f)) {
                        part.transform.scaleX = scaleValues[0];
                        part.transform.scaleY = scaleValues[1];
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
                float newAngle = part.transform.angle;

                if (ImGui::SliderFloat("Angle Z", &newAngle, -360.f, 360.f, "%.1f deg"))
                    part.transform.angle = newAngle;

                if (ImGui::IsItemActivated())
                    oldAngle = originalPart.transform.angle;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.transform.angle = oldAngle;
                    newPart.transform.angle = newAngle;
                }
            }

            ImGui::Checkbox("Flip X", &newPart.flipX);
            ImGui::Checkbox("Flip Y", &newPart.flipY);

            ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Rendering");

            // Opacity slider
            {
                static const uint8_t min { 0 };
                static const uint8_t max { 0xFF };

                static uint8_t oldOpacity { 0 };
                uint8_t newOpacity = part.opacity;

                if (ImGui::SliderScalar("Opacity", ImGuiDataType_U8, &newOpacity, &min, &max, "%u"))
                    part.opacity = newOpacity;

                if (ImGui::IsItemActivated())
                    oldOpacity = originalPart.opacity;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.opacity = oldOpacity;
                    newPart.opacity = newOpacity;
                }
            }

            // Fore Color
            if (isCtr) {
                static CellAnim::CTRColor oldForeColor { 0.f, 0.f, 0.f };

                CellAnim::CTRColor newForeColor = part.foreColor;

                if (ImGui::ColorEdit3("Fore Color", &newForeColor.r))
                    part.foreColor = newForeColor;

                if (ImGui::IsItemActivated())
                    oldForeColor = originalPart.foreColor;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.foreColor = oldForeColor;
                    newPart.foreColor = newForeColor;
                }
            }

            // Back Color
            if (isCtr) {
                static CellAnim::CTRColor oldBackColor { 0.f, 0.f, 0.f };

                CellAnim::CTRColor newBackColor = part.backColor;

                if (ImGui::ColorEdit3("Back Color", &newBackColor.r))
                    part.backColor = newBackColor;

                if (ImGui::IsItemActivated())
                    oldBackColor = originalPart.backColor;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.backColor = oldBackColor;
                    newPart.backColor = newBackColor;
                }
            }

            ImGui::SeparatorText((const char*)ICON_FA_BORDER_TOP_LEFT " Region");
            {
                // Position XY
                {
                    static int oldPosition[2] { 0, 0 };
                    int positionValues[2] {
                        static_cast<int>(part.regionX),
                        static_cast<int>(part.regionY)
                    };

                    if (ImGui::DragInt2(
                        "Position XY##Region", positionValues, 1.f,
                        0, CellAnim::ArrangementPart::MAX_REGION
                    )) {
                        part.regionX = positionValues[0];
                        part.regionY = positionValues[1];
                    }

                    if (ImGui::IsItemActivated()) {
                        oldPosition[0] = originalPart.regionX;
                        oldPosition[1] = originalPart.regionY;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        originalPart.regionX = oldPosition[0];
                        originalPart.regionY = oldPosition[1];

                        newPart.regionX = positionValues[0];
                        newPart.regionY = positionValues[1];
                    }
                }

                // Size WH
                {
                    static int oldSize[2] { 0, 0 };
                    int sizeValues[2] {
                        static_cast<int>(newPart.regionW),
                        static_cast<int>(newPart.regionH)
                    };

                    if (ImGui::DragInt2(
                        "Size WH##Region", sizeValues, 1.f,
                        0, CellAnim::ArrangementPart::MAX_REGION
                    )) {
                        part.regionW = sizeValues[0];
                        part.regionH = sizeValues[1];
                    }

                    if (ImGui::IsItemActivated()) {
                        oldSize[0] = originalPart.regionW;
                        oldSize[1] = originalPart.regionH;
                    }

                    if (ImGui::IsItemDeactivated()) {
                        originalPart.regionW = oldSize[0];
                        originalPart.regionH = oldSize[1];

                        newPart.regionW = sizeValues[0];
                        newPart.regionH = sizeValues[1];
                    }
                }
            }

            // ID
            if (isCtr) {
                ImGui::SeparatorText((const char*)ICON_FA_PENCIL " ID");

                static int oldID { 0 };
                int newID = part.id;

                if (ImGui::InputInt("ID", &newID))
                    part.id = std::clamp<int>(newID, 0, CellAnim::ArrangementPart::MAX_ID);

                if (ImGui::IsItemActivated())
                    oldID = originalPart.id;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.id = oldID;
                    newPart.id = std::clamp<int>(newID, 0, CellAnim::ArrangementPart::MAX_ID);
                }
            }

            // Emitter Name
            if (isCtr) {
                ImGui::SeparatorText((const char*)ICON_FA_WAND_MAGIC_SPARKLES " Effects");

                static std::string oldEmitterName {};

                std::string newEmitterName = part.emitterName;

                if (textInputStdString("Emitter Name", newEmitterName))
                    part.emitterName = newEmitterName;

                if (ImGui::IsItemActivated())
                    oldEmitterName = originalPart.emitterName;

                if (ImGui::IsItemDeactivated()) {
                    originalPart.emitterName = oldEmitterName;
                    newPart.emitterName = newEmitterName;
                }
            }

            if (newPart != originalPart) {
                part = originalPart;

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifyArrangementPart>(
                    sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                    playerManager.getArrangementIndex(),
                    appState.selectedParts.at(0).index,
                    newPart
                ));
            }

            ImGui::EndDisabled();
        }
        else {
            if (appState.multiplePartsSelected())
                ImGui::TextWrapped("Multiple parts selected -- inspector unavaliable."); // TODOLT: implement
            else
                ImGui::TextWrapped("No part selected.");
        }
    }
    ImGui::EndChild();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

    ImGui::BeginChild("ArrangementParts", { 0.f, 0.f }, ImGuiChildFlags_Borders);
    ImGui::PopStyleVar();
    {
        ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Parts");

        auto& arrangement = playerManager.getArrangement();

        int selDeleteSingle = -1; // Index of part to delete or <0.
        int insertNewPart = -1; // Index of part to insert or <0.

        if (!arrangement.parts.empty()) {
            ImGui::BeginChild("PartList", { 0.f, 0.f }, ImGuiChildFlags_None);

            const ImVec2 listChildSize = ImGui::GetContentRegionAvail();

            ImGuiMultiSelectFlags msFlags =
                ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
                ImGuiMultiSelectFlags_BoxSelect1d;
            ImGuiMultiSelectIO* msIo = ImGui::BeginMultiSelect(msFlags, appState.selectedParts.size(), arrangement.parts.size());

            appState.processMultiSelectRequests(msIo);

            bool wantDeleteSelected = (
                ImGui::Shortcut(ImGuiKey_Backspace | ImGuiMod_Ctrl, ImGuiInputFlags_Repeat) ||
                ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat)
            ) && appState.anyPartsSelected();

            int itemCurrentIndexToFocus = wantDeleteSelected ?
                appState.getNextPartIndexAfterDeletion(msIo, arrangement.parts.size()) :
                -1;

            for (int n = arrangement.parts.size() - 1; n >= 0; n--) {
                ImGui::PushID(n);

                auto& part = arrangement.parts[n];

                char selectableLabel[128];
                if (part.editorName.empty())
                    snprintf(
                        selectableLabel, sizeof(selectableLabel),
                        "Part no. %u", n+1
                    );
                else
                    snprintf(
                        selectableLabel, sizeof(selectableLabel),
                        "Part no. %u (%s)", n+1, part.editorName.c_str()
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
                            const auto& part = arrangement.parts.at(index);

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
                            auto newArrangement = playerManager.getArrangement();
                            newArrangement.parts.insert(
                                newArrangement.parts.begin() + n + 1,
                                copyParts.begin(),
                                copyParts.end()
                            );

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangement>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getArrangementIndex(),
                                newArrangement
                            ));

                            appState.clearSelectedParts();
                            for (unsigned i = 0; i < copyParts.size(); i++)
                                appState.setPartSelected(n + 1 + i, true);
                            //appState.setPartSelected(n + 1, true);
                        }
                        if (ImGui::MenuItem("..below")) {
                            auto newArrangement = playerManager.getArrangement();
                            newArrangement.parts.insert(
                                newArrangement.parts.begin() + n,
                                copyParts.begin(),
                                copyParts.end()
                            );

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangement>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getArrangementIndex(),
                                newArrangement
                            ));

                            appState.clearSelectedParts();
                            for (unsigned i = 0; i < copyParts.size(); i++)
                                appState.setPartSelected(n + i, true);
                            //appState.setPartSelected(n, true);
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("..here (replace)")) {
                            auto newArrangement = playerManager.getArrangement();

                            std::sort(
                                appState.selectedParts.begin(), appState.selectedParts.end(),
                                [](const AppState::SelectedPart& a, const AppState::SelectedPart& b) {
                                    return a.index > b.index;
                                }
                            );
                            for (const auto& [index, _] : appState.selectedParts) {
                                if (index < newArrangement.parts.size())
                                    newArrangement.parts.erase(newArrangement.parts.begin() + index);
                            }

                            n = std::clamp(n, 0, static_cast<int>(newArrangement.parts.size()));

                            newArrangement.parts.insert(
                                newArrangement.parts.begin() + n,
                                copyParts.begin(),
                                copyParts.end()
                            );

                            sessionManager.getCurrentSession()->addCommand(
                                std::make_shared<CommandModifyArrangement>(
                                    sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                    playerManager.getArrangementIndex(),
                                    newArrangement
                                )
                            );

                            appState.clearSelectedParts();
                            for (unsigned i = 0; i < copyParts.size(); i++) {
                                if (n + i < newArrangement.parts.size())
                                    appState.setPartSelected(n + i, true);
                            }
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Paste single part (special)..", copyParts.size() == 1)) {
                        if (ImGui::MenuItem("..transform")) {
                            CellAnim::ArrangementPart newPart = part;

                            newPart.transform = copyParts[0].transform;

                            newPart.flipX = copyParts[0].flipX;
                            newPart.flipY = copyParts[0].flipY;

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangementPart>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getArrangementIndex(),
                                n,
                                newPart
                            ));

                            appState.clearSelectedParts();
                            appState.setPartSelected(n, true);
                        }

                        if (ImGui::MenuItem("..opacity")) {
                            CellAnim::ArrangementPart newPart = part;
                            newPart.opacity = copyParts[0].opacity;

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangementPart>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getArrangementIndex(),
                                n,
                                newPart
                            ));

                            appState.clearSelectedParts();
                            appState.setPartSelected(n, true);
                        }

                        if (ImGui::MenuItem("..region")) {
                            CellAnim::ArrangementPart newPart = part;

                            newPart.regionX = copyParts[0].regionX;
                            newPart.regionY = copyParts[0].regionY;

                            newPart.regionW = copyParts[0].regionW;
                            newPart.regionH = copyParts[0].regionH;

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangementPart>(
                                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                                playerManager.getArrangementIndex(),
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
                                std::sort(
                                    appState.selectedParts.begin(), appState.selectedParts.end(),
                                    [](const AppState::SelectedPart& a, const AppState::SelectedPart& b) {
                                        return a.index < b.index;
                                    }
                                );

                                copyParts[i] = arrangement.parts.at(
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
                                appState.getNextPartIndexAfterDeletion(msIo, arrangement.parts.size());
                        }
                        else
                            selDeleteSingle = n;
                    }

                    ImGui::EndDisabled();

                    ImGui::EndPopup();
                }

                ImGui::SameLine(4.f, 0.f);
                if (partToggleButton((const char*)ICON_FA_EYE "##Visible", !part.editorVisible)) {
                    if (!isPartSelected)
                        part.editorVisible ^= true;
                    else {
                        for (const auto& [index, _] : appState.selectedParts)
                            arrangement.parts.at(index).editorVisible ^= true;
                    }
                }

                ImGui::SameLine();
                if (partToggleButton((const char*)ICON_FA_LOCK "##Locked", part.editorLocked)) {
                    if (!isPartSelected)
                        part.editorLocked ^= true;
                    else {
                        for (const auto& [index, _] : appState.selectedParts)
                            arrangement.parts.at(index).editorLocked ^= true;
                    }
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(part.editorLocked);
                ImGui::TextUnformatted(selectableLabel);
                ImGui::EndDisabled();

                const ImGuiStyle& style = ImGui::GetStyle();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.f, style.ItemSpacing.y });

                float firstButtonWidth = ImGui::CalcTextSize((const char*)ICON_FA_ARROW_DOWN "").x + (style.FramePadding.x * 2);
                float basePositionX = listChildSize.x - (style.FramePadding.x * 2) - style.ScrollbarSize;

                ImGui::SameLine();
                ImGui::SetCursorPosX(basePositionX - firstButtonWidth - style.ItemSpacing.x);

                ImGui::BeginDisabled(part.editorLocked);

                ImGui::BeginDisabled(n == static_cast<int>(arrangement.parts.size()) - 1);
                if (ImGui::SmallButton((const char*)ICON_FA_ARROW_UP "")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveArrangementPart>(
                        sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                        playerManager.getArrangementIndex(),
                        n,
                        false
                    ));
                }
                ImGui::EndDisabled();

                ImGui::SameLine();
                ImGui::SetCursorPosX(basePositionX);

                ImGui::BeginDisabled(n == 0);
                if (ImGui::SmallButton((const char*)ICON_FA_ARROW_DOWN "")) {
                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandMoveArrangementPart>(
                        sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                        playerManager.getArrangementIndex(),
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
                auto newArrangement = arrangement;
                appState.deleteSelectedParts(msIo, newArrangement.parts, itemCurrentIndexToFocus);

                sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandModifyArrangement>(
                        sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                        playerManager.getArrangementIndex(),
                        newArrangement
                    )
                );
            }
        }
        else {
            if (ImGui::Selectable("Click here to create a new part."))
                insertNewPart = 0;
        }

        if (selDeleteSingle >= 0) {
            sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandDeleteArrangementPart>(
                    sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                    playerManager.getArrangementIndex(),
                    selDeleteSingle
                )
            );
        }

        if (insertNewPart >= 0) {
            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandInsertArrangementPart>(
                sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
                playerManager.getArrangementIndex(),
                insertNewPart,
                CellAnim::ArrangementPart {}
            ));

            appState.clearSelectedParts();
            appState.setPartSelected(insertNewPart, true);
        }
    }
    ImGui::EndChild();
}
