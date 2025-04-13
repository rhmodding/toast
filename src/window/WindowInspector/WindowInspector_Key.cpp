#include "../WindowInspector.hpp"

#include "../../SessionManager.hpp"

#include "../../ThemeManager.hpp"

#include "../../command/CommandModifyAnimationKey.hpp"

#include "../../font/FontAwesome.h"

#include "../../AppState.hpp"

template <typename T>
void valueEditor(
    const char* label, T& currentValue,
    std::function<T()> getOriginal,
    std::function<void(const T& oldValue, const T& newValue)> setFinal,
    std::function<bool(const char* label, T* value)> widgetCall
) {
    static T oldValue;
    T tempValue = currentValue;

    if (widgetCall(label, &tempValue))
        currentValue = tempValue;

    if (ImGui::IsItemActivated())
        oldValue = getOriginal();
    if (ImGui::IsItemDeactivated())
        setFinal(oldValue, tempValue);
}

void WindowInspector::Level_Key() {
    AppState& appState = AppState::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();
    PlayerManager& playerManager = PlayerManager::getInstance();

    this->drawPreview();

    const bool isCtr = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_CTR;

    ImGui::SameLine();

    const auto& arrangements = sessionManager.getCurrentSession()
        ->getCurrentCellanim().object->arrangements;

    CellAnim::AnimationKey newKey = playerManager.getKey();
    CellAnim::AnimationKey originalKey = playerManager.getKey();

    unsigned animationIndex = playerManager.getAnimationIndex();
    const char* animationName = playerManager.getAnimation().name.c_str();
    if (animationName[0] == '\0')
        animationName = "(no name set)";

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Anim \"%s\" (no. %u)", animationName, animationIndex+1);

        ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
        ImGui::TextWrapped("Key no. %u", playerManager.getKeyIndex() + 1);
        ImGui::PopFont();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    CellAnim::AnimationKey& animKey = playerManager.getKey();

    ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Arrangement");

    // Arrangement Input
    {
        static int oldArrangement { 0 };
        int newArrangement = playerManager.getArrangementIndex() + 1;

        ImGui::SetNextItemWidth(
            ImGui::CalcItemWidth() -
            (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x) * 2
        );

        if (ImGui::InputInt("##Arrangement No.", &newArrangement)) {
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

        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextUnformatted("Arrangement No.");
    }

    this->duplicateArrangementButton(newKey, originalKey);

    ImGui::SeparatorText((const char*)ICON_FA_PAUSE " Hold");

    valueEditor<unsigned>("Hold Frames", animKey.holdFrames,
        [&]() { return originalKey.holdFrames; },
        [&](const unsigned& oldValue, const unsigned& newValue) {
            originalKey.holdFrames = oldValue;
            newKey.holdFrames = std::clamp<int>(static_cast<int>(newValue),
                0, CellAnim::AnimationKey::MAX_HOLD_FRAMES
            );
        },
        [](const char* label, unsigned* value) {
            bool changed = ImGui::InputInt(label, reinterpret_cast<int*>(value));
            if (changed) {
                *value = std::clamp<int>(static_cast<int>(*value),
                    0, CellAnim::AnimationKey::MAX_HOLD_FRAMES
                );
            }
            return changed;
        }
    );

    ImGui::BulletText("A hold frames value of 0 will cause this key to be skipped.");
    ImGui::Dummy({ 0.f, 1.f });

    ImGui::SeparatorText((const char*)ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform");

    // Position XY
    {
        static int oldPosition[2] { 0, 0 };
        int positionValues[2] {
            animKey.transform.positionX,
            animKey.transform.positionY
        };

        if (ImGui::DragInt2(
            "Position XY",
            positionValues, 1.f,
            CellAnim::TransformValues::MIN_POSITION,
            CellAnim::TransformValues::MAX_POSITION
        )) {
            animKey.transform.positionX = positionValues[0];
            animKey.transform.positionY = positionValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldPosition[0] = originalKey.transform.positionX;
            oldPosition[1] = originalKey.transform.positionY;
        }

        if (ImGui::IsItemDeactivated()) {
            originalKey.transform.positionX = oldPosition[0];
            originalKey.transform.positionY = oldPosition[1];

            newKey.transform.positionX = positionValues[0];
            newKey.transform.positionY = positionValues[1];
        }
    }

    // Scale XY
    {
        static float oldScale[2] { 0.f, 0.f };
        float scaleValues[2] { animKey.transform.scaleX, animKey.transform.scaleY };

        if (ImGui::DragFloat2("Scale XY", scaleValues, .01f)) {
            animKey.transform.scaleX = scaleValues[0];
            animKey.transform.scaleY = scaleValues[1];
        }

        if (ImGui::IsItemActivated()) {
            oldScale[0] = originalKey.transform.scaleX;
            oldScale[1] = originalKey.transform.scaleY;
        }

        if (ImGui::IsItemDeactivated()) {
            originalKey.transform.scaleX = oldScale[0];
            originalKey.transform.scaleY = oldScale[1];

            newKey.transform.scaleX = scaleValues[0];
            newKey.transform.scaleY = scaleValues[1];
        }
    }

    valueEditor<float>("Angle Z", animKey.transform.angle,
        [&]() { return originalKey.transform.angle; },
        [&](const float& oldValue, const float& newValue) {
            originalKey.transform.angle = oldValue;
            newKey.transform.angle = newValue;
        },
        [](const char* label, float* value) {
            return ImGui::SliderFloat(label, value, -360.f, 360.f, "%.1f deg");
        }
    );

    // Translate Z
    if (isCtr) {
        ImGui::Dummy({ 0.f, 3.f });

        valueEditor<float>("Position Z", animKey.translateZ,
            [&]() { return originalKey.translateZ; },
            [&](const float& oldValue, const float& newValue) {
                originalKey.translateZ = oldValue;
                newKey.translateZ = newValue;
            },
            [](const char* label, float* value) {
                return ImGui::InputFloat(label, value);
            }
        );
    }

    ImGui::SeparatorText((const char*)ICON_FA_IMAGE " Rendering");

    valueEditor<uint8_t>("Opacity", animKey.opacity,
        [&]() { return originalKey.opacity; },
        [&](const uint8_t& oldValue, const uint8_t& newValue) {
            originalKey.opacity = oldValue;
            newKey.opacity = newValue;
        },
        [](const char* label, uint8_t* value) {
            static const uint8_t min { 0 };
            static const uint8_t max { 0xFF };

            return ImGui::SliderScalar(label, ImGuiDataType_U8, value, &min, &max, "%u");
        }
    );

    // Fore & Back Color
    if (isCtr) {
        valueEditor<CellAnim::CTRColor>("Fore Color", animKey.foreColor,
            [&]() { return originalKey.foreColor; },
            [&](const CellAnim::CTRColor& oldValue, const CellAnim::CTRColor& newValue) {
                originalKey.foreColor = oldValue;
                newKey.foreColor = newValue;
            },
            [](const char* label, CellAnim::CTRColor* value) {
                return ImGui::ColorEdit3(label, &value->r);
            }
        );

        valueEditor<CellAnim::CTRColor>("Back Color", animKey.backColor,
            [&]() { return originalKey.backColor; },
            [&](const CellAnim::CTRColor& oldValue, const CellAnim::CTRColor& newValue) {
                originalKey.backColor = oldValue;
                newKey.backColor = newValue;
            },
            [](const char* label, CellAnim::CTRColor* value) {
                return ImGui::ColorEdit3(label, &value->r);
            }
        );
    }

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
