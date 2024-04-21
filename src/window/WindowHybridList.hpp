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

    std::unordered_map<uint16_t, std::string>* animationNames;

private:
    bool flashWindow{ false };
    bool flashTrigger{ false };
    float flashTimer{ 0.f };

    void FlashWindow();
    void ResetFlash();
};

#endif // WINDOWHYBRIDLIST_HPP