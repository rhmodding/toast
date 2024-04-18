#ifndef ANIMATABLE_HPP
#define ANIMATABLE_HPP

#include <cstdint>

#include "RevCellAnim.hpp"

#include "../common.hpp"

#include <imgui.h>

class Animatable {
private:
    bool animating{ false };

    RevCellAnim::Animation* currentAnimation{ nullptr };
    RevCellAnim::AnimationKey* currentKey{ nullptr };

    int16_t holdKey{ 0 };

    uint16_t currentAnimationIndex{ 0 };
    uint16_t currentKeyIndex{ 0 };

public:
    RevCellAnim::RevCellAnimObject* cellanim{ nullptr };
    Common::Image* texture{ nullptr };

    ImVec2 offset{ 512, 512 };

    float scaleX{ 1.0f };
    float scaleY{ 1.0f };

    bool visible{ true };

    Animatable(RevCellAnim::RevCellAnimObject* cellanim, Common::Image* texture) : cellanim(cellanim), texture(texture) {}

    ~Animatable() {};

    void setAnimation(uint16_t animIndex);
    void setAnimationKey(uint16_t keyIndex);

    void setAnimating(bool animating);
    bool isAnimating() const;

    uint16_t getCurrentAnimationIndex() const;
    uint16_t getCurrentKeyIndex() const;

    int16_t getHoldFramesLeft() const;

    RevCellAnim::AnimationKey* getCurrentKey() const;
    RevCellAnim::Animation* getCurrentAnimation() const;

    void Update();

    void Draw(ImDrawList* drawList, bool allowOpacity = true);

    void DrawOrigins(ImDrawList* drawList, float radius, uint32_t color);

    // Does the Animatable actually draw anything at all to the screen?
    bool getDoesDraw(bool allowOpacity = true) const;

    ImVec2 getPartWorldSpace(uint16_t animationIndex, uint16_t keyIndex, uint16_t partIndex) const;
};

#endif // ANIMATABLE_HPP