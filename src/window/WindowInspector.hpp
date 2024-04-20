#ifndef WINDOWINSPECTOR_HPP
#define WINDOWINSPECTOR_HPP

#include "BaseWindow.hpp"

#include "../anim/Animatable.hpp"

#include <unordered_map>
#include <string>

class WindowInspector : public BaseWindow {
public:
    void Update() override;

    Animatable* animatable;
    std::unordered_map<uint16_t, std::string>* animationNames;

    enum InspectorLevel: uint8_t {
        InspectorLevel_Animation,
        InspectorLevel_Key,
        InspectorLevel_Arrangement,
        InspectorLevel_Arrangement_Im
    } inspectorLevel{ InspectorLevel_Animation };

private:
    void Level_Animation();
    void Level_Key();
    void Level_Arrangement();
};

#endif // WINDOWINSPECTOR_HPP