#include "window/WindowCellQuickSel.hpp"

#include "window/WindowCanvas.hpp"

#include "manager/SessionManager.hpp"
#include "manager/PlayerManager.hpp"

#include "command/CompositeCommand.hpp"
#include "command/CommandModifyArrangementPart.hpp"

#include "SelectionState.hpp"

#include <imgui_internal.h>

static bool cellSelectable(const char* label, ImVec2 size, ImVec2 cellOrigin, ImVec2 cellSize) {
    constexpr ImGuiButtonFlags BUTTON_FLAGS = ImGuiButtonFlags_MouseButtonLeft;
    constexpr float BOTTOM_PAD = 14;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiID id = window->GetID(label);

    const ImGuiStyle& style = ImGui::GetStyle();

    const ImVec2 padding = style.FramePadding;

    const ImRect bb (window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);

    ImRect bbFull = bb;
    bbFull.Max.y += BOTTOM_PAD;

    ImGui::ItemSize(bbFull);
    if (!ImGui::ItemAdd(bbFull, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bbFull, id, &hovered, &held, BUTTON_FLAGS);

    // Render
    const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImGui::RenderNavCursor(bbFull, id);
    ImGui::RenderFrame(bbFull.Min, bbFull.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));

    uint32_t backgroundCol = WindowCanvas::getBackgroundColor();
    window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, backgroundCol, style.FrameRounding);

    auto cellanimSheet = SessionManager::getInstance().getCurrentSession()->getCurrentCellAnimSheet();

    ImVec2 uvMin = {
        cellOrigin.x / cellanimSheet->getWidth(),
        cellOrigin.y / cellanimSheet->getHeight()
    };
    ImVec2 uvMax = {
        (cellOrigin.x + cellSize.x) / cellanimSheet->getWidth(),
        (cellOrigin.y + cellSize.y) / cellanimSheet->getHeight()
    };

    window->DrawList->AddImage(
        cellanimSheet->getImTextureId(),
        bb.Min + padding, bb.Max - padding,
        uvMin, uvMax
    );

    const char *labelRenderEnd = ImGui::FindRenderedTextEnd(label, nullptr);

    ImVec2 labelSize = ImGui::CalcTextSize(label, labelRenderEnd);
    ImVec2 labelPos = ImVec2((bb.Min.x + bb.Max.x) / 2.0f, bb.Max.y);
    labelPos.x -= labelSize.x / 2.0f;
    labelPos.y -= (labelSize.y / 2.0f) - (BOTTOM_PAD / 2.0f) + 2.0f;

    window->DrawList->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), label, labelRenderEnd);

    return pressed;
}

void WindowCellQuickSel::update() {
    SessionManager &sessionManager = SessionManager::getInstance();

    Session &currentSession = *sessionManager.getCurrentSession();
    Session::CellAnimGroup &curCellAnim = currentSession.getCurrentCellAnim();

    PlayerManager &playerManager = PlayerManager::getInstance();

    SelectionState &selectionState = currentSession.getPartSelectState();
    bool canApply = selectionState.anySelected();
    // auto &part = playerManager.getArrangement().parts.at(selectionState.mSelected[0].index);

    ImGui::Begin("Cell Quick Select", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Cell")) {
            ImGui::BeginDisabled(!selectionState.anySelected());
            if (ImGui::MenuItem("Add from current selection")) {
                for (size_t i = 0; i < selectionState.mSelected.size(); i++) {
                    const auto &part = playerManager.getArrangement().parts.at(selectionState.mSelected[i].index);
                    
                    Session::SavedCell ent;
                    ent.cellOrigin[0] = part.cellOrigin.x;
                    ent.cellOrigin[1] = part.cellOrigin.y;
                    ent.cellSize[0] = part.cellSize.x;
                    ent.cellSize[1] = part.cellSize.y;

                    if (
                        std::find(curCellAnim.mSavedCell.begin(), curCellAnim.mSavedCell.end(), ent) ==
                        curCellAnim.mSavedCell.end()
                    ) {
                        curCellAnim.mSavedCell.push_back(std::move(ent));
                    }
                }
            }
            ImGui::EndDisabled();

            if (ImGui::Selectable("Add manually..", false, ImGuiSelectableFlags_NoAutoClosePopups)) {
                mNewCell.cellOrigin[0] = 8;
                mNewCell.cellOrigin[1] = 8;
                mNewCell.cellSize[0] = 8;
                mNewCell.cellSize[1] = 8;
            }

            bool closeMenu = false;
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft)) {
                static const int min { 0 };
                static const int max { CellAnim::ArrangementPart::MAX_CELL_COORD };

                ImGui::DragScalarN("Origin XY",
                    ImGuiDataType_S32, mNewCell.cellOrigin, 2,
                    1.f, &min, &max,
                    "%d", ImGuiSliderFlags_ClampOnInput
                );

                ImGui::DragScalarN("Size WH",
                    ImGuiDataType_S32, mNewCell.cellSize, 2,
                    1.f, &min, &max,
                    "%d", ImGuiSliderFlags_ClampOnInput
                );

                if (ImGui::Button("Add")) {
                    if (
                        std::find(curCellAnim.mSavedCell.begin(), curCellAnim.mSavedCell.end(), mNewCell) ==
                        curCellAnim.mSavedCell.end()
                    ) {
                        curCellAnim.mSavedCell.push_back(mNewCell);
                    }
                    ImGui::CloseCurrentPopup();
                    closeMenu = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                    closeMenu = true;
                }

                ImGui::EndPopup();
            }

            if (closeMenu) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            ImGui::BeginDisabled(curCellAnim.mSavedCell.empty());
            if (ImGui::MenuItem("Clear all")) {
                curCellAnim.mSavedCell.clear();
            }
            ImGui::EndDisabled();

            ImGui::Separator();

            ImGui::Checkbox("Center on Apply", &mCenterOnApply);

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::SliderInt("Cell Size (px)", &mCellSize, 8, 256);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    const ImVec2 windowSize = ImGui::GetContentRegionAvail();

    float cellSize = static_cast<float>(mCellSize);
    float windowVisibleX = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

    size_t removeIndex = (size_t)-1;
    for (size_t i = 0; i < curCellAnim.mSavedCell.size(); i++) {
        char nameBuf[64];
        std::snprintf(nameBuf, sizeof(nameBuf), "%zu", i);

        Session::SavedCell& ent = curCellAnim.mSavedCell[i];

        bool doApply = cellSelectable(
            nameBuf, ImVec2(cellSize, cellSize),
            ImVec2(ent.cellOrigin[0], ent.cellOrigin[1]), ImVec2(ent.cellSize[0], ent.cellSize[1])
        );

        float lastCellX = ImGui::GetItemRectMax().x;
        float nextCellX = lastCellX + ImGui::GetStyle().ItemSpacing.x + cellSize;
        if ((i + 1 < curCellAnim.mSavedCell.size()) && nextCellX < windowVisibleX) {
            ImGui::SameLine();
        }

        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Left-click to apply, right-click for options.");

            ImGui::EndTooltip();
        }

        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight)) {
            ImGui::BeginDisabled(!canApply);
            if (ImGui::MenuItem("Apply")) {
                doApply = true;
            }
            ImGui::EndDisabled();

            ImGui::Separator();

            if (ImGui::BeginMenu("Modify Cell")) {
                static const int min { 0 };
                static const int max { CellAnim::ArrangementPart::MAX_CELL_COORD };

                ImGui::DragScalarN("Origin XY",
                    ImGuiDataType_S32, ent.cellOrigin, 2,
                    1.f, &min, &max,
                    "%d", ImGuiSliderFlags_ClampOnInput
                );

                ImGui::DragScalarN("Size WH",
                    ImGuiDataType_S32, ent.cellSize, 2,
                    1.f, &min, &max,
                    "%d", ImGuiSliderFlags_ClampOnInput
                );

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Remove")) {
                removeIndex = i;
            }

            ImGui::EndPopup();
        }

        if (doApply) {
            Session &session = *sessionManager.getCurrentSession();
            auto composite = std::make_shared<CompositeCommand>();

            for (size_t i = 0; i < selectionState.mSelected.size(); i++) {
                // Copy
                unsigned partIndex = selectionState.mSelected[i].index;
                auto newPart = playerManager.getArrangement().parts.at(selectionState.mSelected[i].index);

                if (mCenterOnApply) {
                    newPart.transform.position.x += (newPart.cellSize.x - ent.cellSize[0]) / 2.0f;
                    newPart.transform.position.y += (newPart.cellSize.y - ent.cellSize[1]) / 2.0f;
                }

                newPart.cellOrigin.x = ent.cellOrigin[0];
                newPart.cellOrigin.y = ent.cellOrigin[1];
                newPart.cellSize.x = ent.cellSize[0];
                newPart.cellSize.y = ent.cellSize[1];

                composite->addCommand(std::make_shared<CommandModifyArrangementPart>(
                    sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
                    playerManager.getArrangementIndex(),
                    partIndex, newPart
                ));
            }

            session.addCommand(composite);
        }
    }

    if (removeIndex != (size_t)-1) {
        curCellAnim.mSavedCell.erase(curCellAnim.mSavedCell.begin() + removeIndex);
    }

    ImGui::End();
}