#ifndef WINDOWINSPECTOR_HPP
#define WINDOWINSPECTOR_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include "../anim/CellanimRenderer.hpp"

class WindowInspector : public BaseWindow {
public:
    void Update() override;

    enum InspectorLevel {
        InspectorLevel_Animation,
        InspectorLevel_Key,
        InspectorLevel_Arrangement,
        InspectorLevel_Arrangement_Im
    } inspectorLevel { InspectorLevel_Animation };

private:
    void DrawPreview();

    void Level_Animation();
    void Level_Key();
    void Level_Arrangement();

    CellanimRenderer previewRenderer;

    ImVec2 windowSize;
};

#endif // WINDOWINSPECTOR_HPP
