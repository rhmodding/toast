#include "WindowInspector.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

#include "../command/CommandSetArrangementMode.hpp"

void WindowInspector::DrawPreview(Animatable* animatable) {
    GET_WINDOW_DRAWLIST;

    const ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize { 96.f, 96.f };
    const ImVec2 canvasBottomRight { canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y };

    const ImVec2 origin(
        canvasTopLeft.x + static_cast<int>(canvasSize.x / 2),
        canvasTopLeft.y + static_cast<int>(canvasSize.y / 2)
    );

    const uint32_t backgroundColor = AppState::getInstance().getDarkThemeEnabled() ?
        IM_COL32(50, 50, 50, 255) :
        IM_COL32(255, 255, 255, 255);

    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor, ImGui::GetStyle().FrameRounding);

    //drawList->PushClipRect(canvasTopLeft, canvasBottomRight, true);

    ImRect keyRect = animatable->getKeyWorldRect(animatable->getCurrentKey());

    float scaleX = canvasSize.x / (keyRect.GetWidth() * (1 / animatable->scaleX));
    float scaleY = canvasSize.y / (keyRect.GetHeight() * (1 / animatable->scaleY));

    animatable->offset = { 0.f, 0.f };
    animatable->scaleX = std::min(scaleX, scaleY);
    animatable->scaleY = std::min(scaleX, scaleY);

    // Recalculate
    ImVec2 keyCenter = animatable->getKeyWorldRect(animatable->getCurrentKey()).GetCenter();
    animatable->offset = { origin.x - keyCenter.x, origin.y - keyCenter.y };

    animatable->Draw(drawList);

    //drawList->PopClipRect();

    ImGui::Dummy(canvasSize);
}

void WindowInspector::Update() {
    GET_APP_STATE;

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
                    SessionManager::getInstance().getCurrentSession()->executeCommand(
                        std::make_shared<CommandSetArrangementMode>(false)
                    );

                appState.focusOnSelectedPart = false;
            }
            if (ImGui::MenuItem("Key", nullptr, inspectorLevel == InspectorLevel_Key)) {
                inspectorLevel = InspectorLevel_Key;

                if (appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->executeCommand(
                        std::make_shared<CommandSetArrangementMode>(false)
                    );

                appState.focusOnSelectedPart = false;
            }
            if (ImGui::MenuItem("Arrangement (Immediate)", nullptr, inspectorLevel == InspectorLevel_Arrangement_Im)) {
                inspectorLevel = InspectorLevel_Arrangement_Im;

                if (appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->executeCommand(
                        std::make_shared<CommandSetArrangementMode>(false)
                    );

                appState.focusOnSelectedPart = true;
            }
            if (ImGui::MenuItem("Arrangement (Outside Anim)", nullptr, inspectorLevel == InspectorLevel_Arrangement)) {
                inspectorLevel = InspectorLevel_Arrangement;

                if (!appState.getArrangementMode())
                    SessionManager::getInstance().getCurrentSession()->executeCommand(
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
        ImGui::Text("Inspector level not implemented.");
        break;
    }

    ImGui::End();
}
