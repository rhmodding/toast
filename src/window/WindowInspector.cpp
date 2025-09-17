#include "WindowInspector.hpp"

#include "manager/SessionManager.hpp"

#include "manager/AppState.hpp"

#include "manager/ThemeManager.hpp"

#include "command/CommandSetArrangementMode.hpp"

void WindowInspector::drawPreview() {
    const PlayerManager& playerManager = PlayerManager::getInstance();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize { 96.f, 96.f };
    const ImVec2 canvasBottomRight { canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y };

    const ImVec2 origin(
        canvasTopLeft.x + static_cast<int>(canvasSize.x / 2),
        canvasTopLeft.y + static_cast<int>(canvasSize.y / 2)
    );

    const uint32_t backgroundColor = ThemeManager::getInstance().getThemeIsLight() ?
        IM_COL32(255, 255, 255, 255) :
        IM_COL32(50, 50, 50, 255);

    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor, ImGui::GetStyle().FrameRounding);

    drawList->PushClipRect(canvasTopLeft, canvasBottomRight, true);

    const auto& currentSession = SessionManager::getInstance().getCurrentSession();

    mBoxPreviewRenderer.linkCellAnim(currentSession->getCurrentCellAnim().object);
    mBoxPreviewRenderer.linkTextureGroup(currentSession->sheets);

    mBoxPreviewRenderer.setScale(ImVec2(1.f, 1.f));

    ImRect keyRect = mBoxPreviewRenderer.getKeyWorldRect(playerManager.getKey());

    float scaleX = canvasSize.x / keyRect.GetWidth();
    float scaleY = canvasSize.y / keyRect.GetHeight();

    mBoxPreviewRenderer.setOffset(ImVec2(0.f, 0.f));
    mBoxPreviewRenderer.setScale(ImVec2(std::min(scaleX, scaleY), std::min(scaleX, scaleY)));

    // Recalculate
    ImVec2 keyCenter = mBoxPreviewRenderer.getKeyWorldRect(playerManager.getKey()).GetCenter();
    mBoxPreviewRenderer.setOffset(ImVec2(origin.x - keyCenter.x, origin.y - keyCenter.y));

    mBoxPreviewRenderer.Draw(drawList, playerManager.getAnimation(), playerManager.getKeyIndex());

    drawList->PopClipRect();

    ImGui::Dummy(canvasSize);
}

void WindowInspector::duplicateArrangementButton(CellAnim::AnimationKey& newKey, const CellAnim::AnimationKey& originalKey) {
    const PlayerManager& playerManager = PlayerManager::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();

    auto& cellAnim = *sessionManager.getCurrentSession()->getCurrentCellAnim().object;

    size_t usageCount = cellAnim.countArrangementUses(playerManager.getArrangementIndex());
    ImGui::BeginDisabled(usageCount <= 1);

    if (ImGui::Button("Make arrangement unique (duplicate)")) {
        newKey.arrangementIndex = cellAnim.duplicateArrangement(originalKey.arrangementIndex);
    }

    ImGui::EndDisabled();

    if (
        (usageCount <= 1) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)
    )
        ImGui::OpenPopup("DuplicateAnywayPopup");

    if (ImGui::BeginPopup("DuplicateAnywayPopup")) {
        ImGui::TextUnformatted("Would you like to duplicate this\narrangement anyway?");
        ImGui::Separator();

        if (ImGui::Selectable("OK")) {
            newKey.arrangementIndex = cellAnim.duplicateArrangement(originalKey.arrangementIndex);
        }
        ImGui::Selectable("Nevermind");

        ImGui::EndPopup();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (usageCount == 0) {
            ImGui::SetTooltip("This arrangement is only used once.\nYou can right-click to duplicate anyway.");
        }
        else if (usageCount == 1) {
            ImGui::SetTooltip("This arrangement isn't used.\nYou can right-click to duplicate anyway.");
        }
    }
}

void WindowInspector::Update() {
    ImGuiWindowFlags windowFlag = ImGuiWindowFlags_MenuBar;
    if (
        mInspectorLevel == InspectorLevel_Arrangement ||
        mInspectorLevel == InspectorLevel_Arrangement_Im
    )
        windowFlag |= ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("Inspector", nullptr, windowFlag);

    mWindowSize = ImGui::GetContentRegionAvail();

    auto session = SessionManager::getInstance().getCurrentSession();
    const bool arrangementMode = session->arrangementMode;

    // Sync mInspectorLevel with arrangementMode if needed
    if (mInspectorLevel == InspectorLevel_Arrangement && !arrangementMode)
        mInspectorLevel = InspectorLevel_Arrangement_Im;
    else if (mInspectorLevel != InspectorLevel_Arrangement && arrangementMode)
        mInspectorLevel = InspectorLevel_Arrangement;

    if (ImGui::BeginMenuBar()) {
        InspectorLevel newLevel = mInspectorLevel;
        bool newFocus = AppState::getInstance().mFocusOnSelectedPart;

        if (ImGui::MenuItem("Animation", nullptr, mInspectorLevel == InspectorLevel_Animation)) {
            newLevel = InspectorLevel_Animation;
            newFocus = false;
        }
        if (ImGui::MenuItem("Key", nullptr, mInspectorLevel == InspectorLevel_Key)) {
            newLevel = InspectorLevel_Key;
            newFocus = false;
        }
        if (ImGui::MenuItem("Arrangement", nullptr, mInspectorLevel == InspectorLevel_Arrangement_Im)) {
            newLevel = InspectorLevel_Arrangement_Im;
            newFocus = true;
        }
        if (ImGui::MenuItem("Arrangement (All)", nullptr, mInspectorLevel == InspectorLevel_Arrangement)) {
            newLevel = InspectorLevel_Arrangement;
            newFocus = true;
        }

        if (newLevel != mInspectorLevel) {
            bool changingToArrangementMode = (newLevel == InspectorLevel_Arrangement);
            if (arrangementMode != changingToArrangementMode) {
                session->addCommand(
                    std::make_shared<CommandSetArrangementMode>(changingToArrangementMode)
                );
            }

            mInspectorLevel = newLevel;
            AppState::getInstance().mFocusOnSelectedPart = newFocus;
        }

        ImGui::EndMenuBar();
    }

    if (arrangementMode || mInspectorLevel == InspectorLevel_Arrangement_Im) {
        Level_Arrangement();
    }
    else {
        switch (mInspectorLevel) {
        case InspectorLevel_Animation:
            Level_Animation();
            break;
        case InspectorLevel_Key:
            Level_Key();
            break;

        default:
            ImGui::TextUnformatted("Inspector level not implemented.");
            break;
        }
    }


    ImGui::End();
}
