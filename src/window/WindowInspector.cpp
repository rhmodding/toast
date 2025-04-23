#include "WindowInspector.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

#include "../ThemeManager.hpp"

#include "../cellanim/CellAnimHelpers.hpp"

#include "../command/CommandSetArrangementMode.hpp"

void WindowInspector::drawPreview() {
    PlayerManager& playerManager = PlayerManager::getInstance();

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

    this->previewRenderer.linkCellAnim(currentSession->getCurrentCellAnim().object);
    this->previewRenderer.linkTextureGroup(currentSession->sheets);

    this->previewRenderer.scaleX = 1.f;
    this->previewRenderer.scaleY = 1.f;

    ImRect keyRect = this->previewRenderer.getKeyWorldRect(playerManager.getKey());

    float scaleX = canvasSize.x / keyRect.GetWidth();
    float scaleY = canvasSize.y / keyRect.GetHeight();

    this->previewRenderer.offset = { 0.f, 0.f };
    this->previewRenderer.scaleX = std::min(scaleX, scaleY);
    this->previewRenderer.scaleY = this->previewRenderer.scaleX;

    // Recalculate
    ImVec2 keyCenter = this->previewRenderer.getKeyWorldRect(playerManager.getKey()).GetCenter();
    this->previewRenderer.offset = { origin.x - keyCenter.x, origin.y - keyCenter.y };

    this->previewRenderer.Draw(drawList, playerManager.getAnimation(), playerManager.getKeyIndex());

    drawList->PopClipRect();

    ImGui::Dummy(canvasSize);
}

void WindowInspector::duplicateArrangementButton(CellAnim::AnimationKey& newKey, const CellAnim::AnimationKey& originalKey) {
    const PlayerManager& playerManager = PlayerManager::getInstance();

    bool arrangementUnique = CellAnimHelpers::getArrangementUnique(playerManager.getArrangementIndex());
    ImGui::BeginDisabled(arrangementUnique);

    if (ImGui::Button("Make arrangement unique (duplicate)"))
        newKey.arrangementIndex =
            CellAnimHelpers::duplicateArrangement(originalKey.arrangementIndex);

    ImGui::EndDisabled();

    if (
        arrangementUnique &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)
    )
        ImGui::OpenPopup("DuplicateAnywayPopup");

    if (ImGui::BeginPopup("DuplicateAnywayPopup")) {
        ImGui::TextUnformatted("Would you like to duplicate this\narrangement anyway?");
        ImGui::Separator();

        if (ImGui::Selectable("Ok"))
            newKey.arrangementIndex =
                CellAnimHelpers::duplicateArrangement(originalKey.arrangementIndex);
        ImGui::Selectable("Nevermind");

        ImGui::EndPopup();
    }

    if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("This arrangement is only used once.\nYou can right-click to duplicate anyway.");
}

void WindowInspector::Update() {
    SessionManager& sessionManager = SessionManager::getInstance();
    AppState& appState = AppState::getInstance();

    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_MenuBar | (
        (
            inspectorLevel == InspectorLevel_Arrangement ||
            inspectorLevel == InspectorLevel_Arrangement_Im
        ) ?
            ImGuiWindowFlags_NoScrollbar :
            ImGuiWindowFlags_None
    ));

    this->windowSize = ImGui::GetContentRegionAvail();

    auto session = SessionManager::getInstance().getCurrentSession();
    const bool arrangementMode = session->arrangementMode;

    // Sync inspectorLevel with arrangementMode if needed
    if (inspectorLevel == InspectorLevel_Arrangement && !arrangementMode)
        inspectorLevel = InspectorLevel_Arrangement_Im;
    else if (inspectorLevel != InspectorLevel_Arrangement && arrangementMode)
        inspectorLevel = InspectorLevel_Arrangement;

    if (ImGui::BeginMenuBar()) {
        InspectorLevel newLevel = inspectorLevel;
        bool newFocus = appState.focusOnSelectedPart;
    
        if (ImGui::MenuItem("Animation", nullptr, inspectorLevel == InspectorLevel_Animation)) {
            newLevel = InspectorLevel_Animation;
            newFocus = false;
        }
        if (ImGui::MenuItem("Key", nullptr, inspectorLevel == InspectorLevel_Key)) {
            newLevel = InspectorLevel_Key;
            newFocus = false;
        }
        if (ImGui::MenuItem("Arrangement", nullptr, inspectorLevel == InspectorLevel_Arrangement_Im)) {
            newLevel = InspectorLevel_Arrangement_Im;
            newFocus = true;
        }
        if (ImGui::MenuItem("Arrangement (All)", nullptr, inspectorLevel == InspectorLevel_Arrangement)) {
            newLevel = InspectorLevel_Arrangement;
            newFocus = true;
        }
    
        if (newLevel != inspectorLevel) {
            bool changingToArrangementMode = (newLevel == InspectorLevel_Arrangement);
            if (arrangementMode != changingToArrangementMode) {
                session->addCommand(
                    std::make_shared<CommandSetArrangementMode>(changingToArrangementMode)
                );
            }
    
            inspectorLevel = newLevel;
            appState.focusOnSelectedPart = newFocus;
        }
    
        ImGui::EndMenuBar();
    }

    if (arrangementMode || inspectorLevel == InspectorLevel_Arrangement_Im) {
        Level_Arrangement();
    }
    else {
        switch (inspectorLevel) {
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
