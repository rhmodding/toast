#ifndef WINDOWHYBRIDLIST_HPP
#define WINDOWHYBRIDLIST_HPP

#include <cstdint>

#include "../AppState.hpp"

#include "BaseWindow.hpp"

#include <unordered_map>
#include <string>

#include "../anim/Animatable.hpp"

class WindowHybridList : public BaseWindow {
public:
    void Update() override;

    Animatable* animatable;

    uint16_t selectedAnimation{ 0 };
    std::unordered_map<uint16_t, std::string>* animationNames;

private:
};

#endif // WINDOWHYBRIDLIST_HPP