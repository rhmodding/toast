#ifndef WINDOWINSPECTOR_HPP
#define WINDOWINSPECTOR_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include "../anim/Animatable.hpp"

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
    bool realPosition { true };

    void DrawPreview(Animatable* animatable);

    void Level_Animation();
    void Level_Key();
    void Level_Arrangement();

    ImVec2 windowSize;
};

#endif // WINDOWINSPECTOR_HPP
