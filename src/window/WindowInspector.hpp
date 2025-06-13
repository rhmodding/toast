#ifndef WINDOW_INSPECTOR_HPP
#define WINDOW_INSPECTOR_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include <functional>

#include "cellanim/CellAnimRenderer.hpp"

class WindowInspector : public BaseWindow {
public:
    void Update() override;

private:
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

    void drawPreview();
    void duplicateArrangementButton(CellAnim::AnimationKey& newKey, const CellAnim::AnimationKey& originalKey);

    void Level_Animation();
    void Level_Key();
    void Level_Arrangement();

private:
    enum InspectorLevel {
        InspectorLevel_Animation,
        InspectorLevel_Key,
        InspectorLevel_Arrangement,
        InspectorLevel_Arrangement_Im
    };

    InspectorLevel mInspectorLevel { InspectorLevel_Animation };

    CellAnimRenderer mBoxPreviewRenderer;

    ImVec2 mWindowSize;
};

#endif // WINDOW_INSPECTOR_HPP
