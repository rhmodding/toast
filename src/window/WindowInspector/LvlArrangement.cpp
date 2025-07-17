#include "../WindowInspector.hpp"

#include <cstdint>

#include "manager/SessionManager.hpp"
#include "manager/ThemeManager.hpp"

#include "command/CommandModifyArrangement.hpp"
#include "command/CommandModifyArrangementPart.hpp"
#include "command/CommandInsertArrangementPart.hpp"
#include "command/CommandDeleteArrangementPart.hpp"
#include "command/CommandMoveArrangementPart.hpp"

#include "command/CommandModifyAnimationKey.hpp"

#include "util/UIUtil.hpp"

#include "font/FontAwesome.h"

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
        { 0.f, 0.f }, labelSize.x, labelSize.y
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

void WindowInspector::Level_Arrangement() {
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    auto& selectionState = sessionManager.getCurrentSession()->getCurrentSelectionState();

    const bool isCtr = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_CTR;

    static std::vector<CellAnim::ArrangementPart> copyParts;

    const auto& arrangements = sessionManager.getCurrentSession()
        ->getCurrentCellAnim().object->getArrangements();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

    ImGui::BeginChild("ArrangementOverview", { 0.f, (ImGui::GetWindowSize().y / 2.f) }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
    ImGui::PopStyleVar();
    {
        drawPreview();

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

                if (ImGui::IsItemDeactivated() && !sessionManager.getCurrentSession()->arrangementMode) {
                    originalKey.arrangementIndex = oldArrangement;
                    newKey.arrangementIndex = std::min<unsigned>(
                        newArrangement - 1, arrangements.size() - 1
                    );
                }

                duplicateArrangementButton(newKey, originalKey);

                if (newKey != originalKey) {
                    playerManager.getKey() = originalKey;

                    sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandModifyAnimationKey>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                        playerManager.getAnimationIndex(),
                        playerManager.getKeyIndex(),
                        newKey
                    ));
                }
            }

            //ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        if (selectionState.singlePartSelected()) {
            CellAnim::ArrangementPart& part = playerManager.getArrangement().parts.at(
                selectionState.mSelectedParts[0].index
            );

            CellAnim::ArrangementPart newPart = part;
            CellAnim::ArrangementPart originalPart = part;

            ImGui::BeginDisabled(part.editorLocked);

            ImGui::SeparatorText((const char*)ICON_FA_PENCIL " Name (editor)");

            UIUtil::Widget::ValueEditor<std::string>("Name", part.editorName,
                [&]() { return originalPart.editorName; },
                [&](const std::string& oldValue, const std::string& newValue) {
                    originalPart.editorName = oldValue;
                    newPart.editorName = newValue;
                },
                [](const char* label, std::string* value) {
                    return UIUtil::Widget::StdStringTextInput(label, *value);
                }
            );

            ImGui::SeparatorText((const char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform");

            UIUtil::Widget::ValueEditor<CellAnim::IntVec2>("Position XY", part.transform.position,
                [&]() { return originalPart.transform.position; },
                [&](const CellAnim::IntVec2& oldValue, const CellAnim::IntVec2& newValue) {
                    originalPart.transform.position = oldValue;
                    newPart.transform.position = newValue;
                },
                [](const char* label, CellAnim::IntVec2* value) {
                    return ImGui::DragInt2(label,
                        value->asArray(), 1.f,
                        CellAnim::TransformValues::MIN_POSITION,
                        CellAnim::TransformValues::MAX_POSITION
                    );
                }
            );

            UIUtil::Widget::ValueEditor<CellAnim::FltVec2>("Scale XY", part.transform.scale,
                [&]() { return originalPart.transform.scale; },
                [&](const CellAnim::FltVec2& oldValue, const CellAnim::FltVec2& newValue) {
                    originalPart.transform.scale = oldValue;
                    newPart.transform.scale = newValue;
                },
                [](const char* label, CellAnim::FltVec2* value) {
                    return ImGui::DragFloat2(label, value->asArray(), .01f);
                }
            );

            UIUtil::Widget::ValueEditor<float>("Angle Z", part.transform.angle,
                [&]() { return originalPart.transform.angle; },
                [&](const float& oldValue, const float& newValue) {
                    originalPart.transform.angle = oldValue;
                    newPart.transform.angle = newValue;
                },
                [](const char* label, float* value) {
                    bool changed = ImGui::SliderFloat(label, value, -360.f, 360.f, "%.1f deg");
                #if defined(__APPLE__)
                    ImGui::SetItemTooltip("Command+Click to input a rotation value.");
                #else
                    ImGui::SetItemTooltip("Ctrl+Click to input a rotation value.");
                #endif

                    return changed;
                }
            );

            ImGui::Checkbox("Flip X", &newPart.flipX);
            ImGui::Checkbox("Flip Y", &newPart.flipY);

            ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Rendering");

            UIUtil::Widget::ValueEditor<uint8_t>("Opacity", part.opacity,
                [&]() { return originalPart.opacity; },
                [&](const uint8_t& oldValue, const uint8_t& newValue) {
                    originalPart.opacity = oldValue;
                    newPart.opacity = newValue;
                },
                [](const char* label, uint8_t* value) {
                    static const uint8_t min { 0 };
                    static const uint8_t max { 0xFF };

                    return ImGui::SliderScalar(label, ImGuiDataType_U8, value, &min, &max, "%u");
                }
            );

            // Fore & Back Color
            if (isCtr) {
                UIUtil::Widget::ValueEditor<CellAnim::CTRColor>("Fore Color", part.foreColor,
                    [&]() { return originalPart.foreColor; },
                    [&](const CellAnim::CTRColor& oldValue, const CellAnim::CTRColor& newValue) {
                        originalPart.foreColor = oldValue;
                        newPart.foreColor = newValue;
                    },
                    [](const char* label, CellAnim::CTRColor* value) {
                        return ImGui::ColorEdit3(label, value->asArray());
                    }
                );

                UIUtil::Widget::ValueEditor<CellAnim::CTRColor>("Back Color", part.backColor,
                    [&]() { return originalPart.backColor; },
                    [&](const CellAnim::CTRColor& oldValue, const CellAnim::CTRColor& newValue) {
                        originalPart.backColor = oldValue;
                        newPart.backColor = newValue;
                    },
                    [](const char* label, CellAnim::CTRColor* value) {
                        return ImGui::ColorEdit3(label, value->asArray());
                    }
                );
            }

            ImGui::SeparatorText((const char*)ICON_FA_BORDER_TOP_LEFT " Cell");

            UIUtil::Widget::ValueEditor<CellAnim::UintVec2>("Origin XY##Cell", part.cellOrigin,
                [&]() { return originalPart.cellOrigin; },
                [&](const CellAnim::UintVec2& oldValue, const CellAnim::UintVec2& newValue) {
                    originalPart.cellOrigin = oldValue;
                    newPart.cellOrigin = newValue;
                },
                [](const char* label, CellAnim::UintVec2* value) {
                    static const uint32_t min { 0 };
                    static const uint32_t max { CellAnim::ArrangementPart::MAX_CELL_COORD };

                    return ImGui::DragScalarN(label,
                        ImGuiDataType_U32, value->asArray(), 2,
                        1.f, &min, &max,
                        "%d", ImGuiSliderFlags_None
                    );
                }
            );

            UIUtil::Widget::ValueEditor<CellAnim::UintVec2>("Size WH##Cell", part.cellSize,
                [&]() { return originalPart.cellSize; },
                [&](const CellAnim::UintVec2& oldValue, const CellAnim::UintVec2& newValue) {
                    originalPart.cellSize = oldValue;
                    newPart.cellSize = newValue;
                },
                [](const char* label, CellAnim::UintVec2* value) {
                    static const uint32_t min { 0 };
                    static const uint32_t max { CellAnim::ArrangementPart::MAX_CELL_COORD };

                    return ImGui::DragScalarN(label,
                        ImGuiDataType_U32, value->asArray(), 2,
                        1.f, &min, &max,
                        "%d", ImGuiSliderFlags_None
                    );
                }
            );

            // ID, Emitter Name, & Quad Depth
            if (isCtr) {
                ImGui::SeparatorText((const char*)ICON_FA_PENCIL " ID");

                UIUtil::Widget::ValueEditor<unsigned>("ID", part.id,
                    [&]() { return originalPart.id; },
                    [&](const unsigned& oldValue, const unsigned& newValue) {
                        originalPart.id = oldValue;
                        newPart.id = std::clamp<int>(static_cast<int>(newValue), 0, CellAnim::ArrangementPart::MAX_ID);
                    },
                    [](const char* label, unsigned* value) {
                        bool changed = ImGui::InputInt(label, reinterpret_cast<int*>(value));
                        if (changed)
                            *value = std::clamp<int>(static_cast<int>(*value), 0, CellAnim::ArrangementPart::MAX_ID);
                        return changed;
                    }
                );

                ImGui::SeparatorText((const char*)ICON_FA_WAND_MAGIC_SPARKLES " 3D Depth");

                UIUtil::Widget::ValueEditor<CellAnim::CTRQuadDepth>("Quad Depth", part.quadDepth,
                    [&]() { return originalPart.quadDepth; },
                    [&](const CellAnim::CTRQuadDepth& oldValue, const CellAnim::CTRQuadDepth& newValue) {
                        originalPart.quadDepth = oldValue;
                        newPart.quadDepth = newValue;
                    },
                    [](const char* label, CellAnim::CTRQuadDepth* value) {
                        bool changed = false;

                        ImGui::PushID(label);

                        ImGui::BeginGroup();

                        float topValues[2] { value->topLeft, value->topRight };
                        if (ImGui::DragFloat2("Top Left, Right", topValues)) {
                            changed = true;
                            value->topLeft = topValues[0];
                            value->topRight = topValues[1];
                        }

                        float bottomValues[2] { value->bottomLeft, value->bottomRight };
                        if (ImGui::DragFloat2("Bottom Left, Right", bottomValues)) {
                            changed = true;
                            value->bottomLeft = bottomValues[0];
                            value->bottomRight = bottomValues[1];
                        }

                        ImGui::EndGroup();

                        ImGui::PopID();

                        return changed;
                    }
                );

                ImGui::SeparatorText((const char*)ICON_FA_WAND_MAGIC_SPARKLES " Effects");

                UIUtil::Widget::ValueEditor<std::string>("Emitter Name", part.emitterName,
                    [&]() { return originalPart.emitterName; },
                    [&](const std::string& oldValue, const std::string& newValue) {
                        originalPart.emitterName = oldValue;
                        newPart.emitterName = newValue;
                    },
                    [](const char* label, std::string* value) {
                        return UIUtil::Widget::StdStringTextInput(label, *value);
                    }
                );
            }

            if (newPart != originalPart) {
                part = originalPart;

                sessionManager.getCurrentSession()->addCommand(
                std::make_shared<CommandModifyArrangementPart>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getArrangementIndex(),
                    selectionState.mSelectedParts.at(0).index,
                    newPart
                ));
            }

            ImGui::EndDisabled();
        }
        else {
            if (selectionState.multiplePartsSelected())
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
            ImGuiMultiSelectIO* msIo = ImGui::BeginMultiSelect(msFlags, selectionState.mSelectedParts.size(), arrangement.parts.size());

            selectionState.processMultiSelectRequests(msIo);

            bool wantDeleteSelected = (
                ImGui::Shortcut(ImGuiKey_Backspace | ImGuiMod_Ctrl, ImGuiInputFlags_Repeat) ||
                ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat)
            ) && selectionState.anyPartsSelected();

            int itemCurrentIndexToFocus = wantDeleteSelected ?
                selectionState.getNextPartIndexAfterDeletion(msIo, arrangement.parts.size()) :
                -1;

            for (int n = arrangement.parts.size() - 1; n >= 0; n--) {
                ImGui::PushID(n);

                auto& part = arrangement.parts[n];

                char selectableLabel[128];
                if (part.editorName.empty())
                    std::snprintf(
                        selectableLabel, sizeof(selectableLabel),
                        "Part no. %u", n+1
                    );
                else
                    std::snprintf(
                        selectableLabel, sizeof(selectableLabel),
                        "Part no. %u (%s)", n+1, part.editorName.c_str()
                    );

                const bool isPartSelected = selectionState.isPartSelected(n);

                ImGui::SetNextItemAllowOverlap();

                ImGui::SetNextItemSelectionUserData(n);
                ImGui::Selectable("###PartSelectable", isPartSelected, 0);
                if (itemCurrentIndexToFocus == n)
                    ImGui::SetKeyboardFocusHere(-1);

                //bool deletePart { false };
                if (ImGui::BeginPopupContextItem()) {
                    char label[128];

                    if (isPartSelected) {
                        std::snprintf(
                            label, sizeof(label),
                            "%zu selected parts",
                            selectionState.mSelectedParts.size()
                        );
                    }
                    else
                        strcpy(label, selectableLabel);

                    ImGui::TextUnformatted(label);

                    // 0 = disabled, 1 = enabled, anything else = mixed
                    int visible = -1;
                    int locked = -1;

                    if (isPartSelected) {
                        for (const auto& [index, _] : selectionState.mSelectedParts) {
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

                    std::snprintf(
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
                                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                playerManager.getArrangementIndex(),
                                newArrangement
                            ));

                            selectionState.clearSelectedParts();
                            for (size_t i = 0; i < copyParts.size(); i++)
                                selectionState.setPartSelected(n + 1 + i, true);
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
                                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                playerManager.getArrangementIndex(),
                                newArrangement
                            ));

                            selectionState.clearSelectedParts();
                            for (size_t i = 0; i < copyParts.size(); i++)
                                selectionState.setPartSelected(n + i, true);
                            //appState.setPartSelected(n, true);
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("..here (replace)")) {
                            auto newArrangement = playerManager.getArrangement();

                            std::sort(
                                selectionState.mSelectedParts.begin(), selectionState.mSelectedParts.end(),
                                [](const SelectionState::SelectedPart& a, const SelectionState::SelectedPart& b) {
                                    return a.index > b.index;
                                }
                            );
                            for (const auto& [index, _] : selectionState.mSelectedParts) {
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
                                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                    playerManager.getArrangementIndex(),
                                    newArrangement
                                )
                            );

                            selectionState.clearSelectedParts();
                            for (size_t i = 0; i < copyParts.size(); i++) {
                                if (n + i < newArrangement.parts.size())
                                    selectionState.setPartSelected(n + i, true);
                            }
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Paste single part (special)..", copyParts.size() == 1)) {
                        if (ImGui::MenuItem(".. transform")) {
                            CellAnim::ArrangementPart newPart = part;

                            newPart.transform = copyParts[0].transform;

                            newPart.flipX = copyParts[0].flipX;
                            newPart.flipY = copyParts[0].flipY;

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangementPart>(
                                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                playerManager.getArrangementIndex(),
                                n,
                                newPart
                            ));

                            selectionState.clearSelectedParts();
                            selectionState.setPartSelected(n, true);
                        }

                        if (ImGui::MenuItem(".. opacity")) {
                            CellAnim::ArrangementPart newPart = part;
                            newPart.opacity = copyParts[0].opacity;

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangementPart>(
                                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                playerManager.getArrangementIndex(),
                                n,
                                newPart
                            ));

                            selectionState.clearSelectedParts();
                            selectionState.setPartSelected(n, true);
                        }

                        if (ImGui::MenuItem(".. cell")) {
                            CellAnim::ArrangementPart newPart = part;

                            newPart.cellOrigin = copyParts[0].cellOrigin;
                            newPart.cellSize = copyParts[0].cellSize;

                            sessionManager.getCurrentSession()->addCommand(
                            std::make_shared<CommandModifyArrangementPart>(
                                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                                playerManager.getArrangementIndex(),
                                n,
                                newPart
                            ));

                            selectionState.clearSelectedParts();
                            selectionState.setPartSelected(n, true);
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::Separator();

                    if (isPartSelected) {
                        std::snprintf(
                            label, sizeof(label),
                            "Copy %zu part%s",
                            selectionState.mSelectedParts.size(),
                            selectionState.mSelectedParts.size() == 1 ? "" : "s"
                        );
                    }
                    else
                        strcpy(label, "Copy part");

                    if (ImGui::Selectable(label)) {
                        if (isPartSelected) {
                            copyParts.resize(selectionState.mSelectedParts.size());
                            for (size_t i = 0; i < selectionState.mSelectedParts.size(); i++) {
                                std::sort(
                                    selectionState.mSelectedParts.begin(), selectionState.mSelectedParts.end(),
                                    [](const SelectionState::SelectedPart& a, const SelectionState::SelectedPart& b) {
                                        return a.index < b.index;
                                    }
                                );

                                copyParts[i] = arrangement.parts.at(
                                    selectionState.mSelectedParts[i].index
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
                        std::snprintf(
                            label, sizeof(label),
                            "Delete %zu part%s",
                            selectionState.mSelectedParts.size(),
                            selectionState.mSelectedParts.size() == 1 ? "" : "s"
                        );
                    }
                    else
                        strcpy(label, "Delete part");

                    if (ImGui::Selectable(label)) {
                        if (isPartSelected) {
                            wantDeleteSelected = true;
                            itemCurrentIndexToFocus = selectionState.getNextPartIndexAfterDeletion(
                                msIo, arrangement.parts.size()
                            );
                        }
                        else
                            selDeleteSingle = n;
                    }

                    ImGui::EndDisabled();

                    ImGui::EndPopup();
                }

                ImGui::SameLine(4.f, 0.f);
                if (partToggleButton((const char*)ICON_FA_EYE "##Visible", !part.editorVisible)) {
                    auto newArrangement = arrangement;

                    if (!isPartSelected)
                        newArrangement.parts[n].editorVisible ^= true;
                    else {
                        for (const auto& [index, _] : selectionState.mSelectedParts)
                            newArrangement.parts.at(index).editorVisible ^= true;
                    }

                    sessionManager.getCurrentSession()->addCommand(
                        std::make_shared<CommandModifyArrangement>(
                            sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                            playerManager.getArrangementIndex(),
                            newArrangement
                        )
                    );
                }

                ImGui::SameLine();
                if (partToggleButton((const char*)ICON_FA_LOCK "##Locked", part.editorLocked)) {
                    auto newArrangement = arrangement;

                    if (!isPartSelected)
                        newArrangement.parts[n].editorLocked ^= true;
                    else {
                        for (const auto& [index, _] : selectionState.mSelectedParts)
                            newArrangement.parts.at(index).editorLocked ^= true;
                    }

                    sessionManager.getCurrentSession()->addCommand(
                        std::make_shared<CommandModifyArrangement>(
                            sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                            playerManager.getArrangementIndex(),
                            newArrangement
                        )
                    );
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
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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
            selectionState.processMultiSelectRequests(msIo);

            ImGui::EndChild();

            if (wantDeleteSelected) {
                auto newArrangement = arrangement;
                selectionState.deleteSelectedParts(msIo, newArrangement.parts, itemCurrentIndexToFocus);

                sessionManager.getCurrentSession()->addCommand(
                    std::make_shared<CommandModifyArrangement>(
                        sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
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
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getArrangementIndex(),
                    selDeleteSingle
                )
            );
        }

        if (insertNewPart >= 0) {
            sessionManager.getCurrentSession()->addCommand(
            std::make_shared<CommandInsertArrangementPart>(
                sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                playerManager.getArrangementIndex(),
                insertNewPart,
                CellAnim::ArrangementPart {}
            ));

            selectionState.clearSelectedParts();
            selectionState.setPartSelected(insertNewPart, true);
        }
    }
    ImGui::EndChild();
}
