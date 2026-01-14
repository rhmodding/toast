#ifndef WINDOW_INSPECTOR_HPP
#define WINDOW_INSPECTOR_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include "cellanim/CellAnimRenderer.hpp"

class WindowInspector : public BaseWindow {
public:
    void update() override;

private:
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
