#include "WindowInspector.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

#include "../ThemeManager.hpp"

#include "../anim/CellanimHelpers.hpp"

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

    this->previewRenderer.linkCellanim(currentSession->getCurrentCellanim().object);
    this->previewRenderer.linkTextureGroup(currentSession->sheets);

    ImRect keyRect = this->previewRenderer.getKeyWorldRect(playerManager.getKey());

    float scaleX = canvasSize.x / (keyRect.GetWidth() * (1.f / this->previewRenderer.scaleX));
    float scaleY = canvasSize.y / (keyRect.GetHeight() * (1.f / this->previewRenderer.scaleY));

    this->previewRenderer.offset = { 0.f, 0.f };
    this->previewRenderer.scaleX = std::min(scaleX, scaleY);
    this->previewRenderer.scaleY = std::min(scaleX, scaleY);

    // Recalculate
    ImVec2 keyCenter = this->previewRenderer.getKeyWorldRect(playerManager.getKey()).GetCenter();
    this->previewRenderer.offset = { origin.x - keyCenter.x, origin.y - keyCenter.y };

    this->previewRenderer.Draw(drawList, playerManager.getAnimation(), playerManager.getKeyIndex());

    drawList->PopClipRect();

    ImGui::Dummy(canvasSize);
}

void WindowInspector::duplicateArrangementButton(CellAnim::AnimationKey& newKey, const CellAnim::AnimationKey& originalKey) {
    const PlayerManager& playerManager = PlayerManager::getInstance();

    bool arrangementUnique = CellanimHelpers::getArrangementUnique(playerManager.getArrangementIndex());
    ImGui::BeginDisabled(arrangementUnique);

    if (ImGui::Button("Make arrangement unique (duplicate)"))
        newKey.arrangementIndex =
            CellanimHelpers::duplicateArrangement(originalKey.arrangementIndex);

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
                CellanimHelpers::duplicateArrangement(originalKey.arrangementIndex);
        ImGui::Selectable("Nevermind");

        ImGui::EndPopup();
    }

    if (arrangementUnique && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("This arrangement is only used once.\nYou can right-click to duplicate anyway.");
}

void WindowInspector::Update() {
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

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Level")) {
            if (ImGui::MenuItem("Animation", nullptr, inspectorLevel == InspectorLevel_Animation)) {
                inspectorLevel = InspectorLevel_Animation;

                if (appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->addCommand(
                        std::make_shared<CommandSetArrangementMode>(false)
                    );

                appState.focusOnSelectedPart = false;
            }
            if (ImGui::MenuItem("Key", nullptr, inspectorLevel == InspectorLevel_Key)) {
                inspectorLevel = InspectorLevel_Key;

                if (appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->addCommand(
                        std::make_shared<CommandSetArrangementMode>(false)
                    );

                appState.focusOnSelectedPart = false;
            }
            if (ImGui::MenuItem("Arrangement (Immediate)", nullptr, inspectorLevel == InspectorLevel_Arrangement_Im)) {
                inspectorLevel = InspectorLevel_Arrangement_Im;

                if (appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->addCommand(
                        std::make_shared<CommandSetArrangementMode>(false)
                    );

                appState.focusOnSelectedPart = true;
            }
            if (ImGui::MenuItem("Arrangement (Outside Anim)", nullptr, inspectorLevel == InspectorLevel_Arrangement)) {
                inspectorLevel = InspectorLevel_Arrangement;

                if (!appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->addCommand(
                        std::make_shared<CommandSetArrangementMode>(true)
                    );

                appState.focusOnSelectedPart = true;
            }

            ImGui::EndMenu();
        }

        /*
        if (ImGui::BeginMenu("Options")) {
            ImGui::EndMenu();
        }
        */

        ImGui::EndMenuBar();
    }

    switch (!appState.getArrangementMode() ? inspectorLevel : InspectorLevel_Arrangement) {
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
        ImGui::TextUnformatted("Inspector level not implemented.");
        break;
    }

    ImGui::End();
}
