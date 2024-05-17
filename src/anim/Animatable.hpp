#ifndef ANIMATABLE_HPP
#define ANIMATABLE_HPP

#include <cstdint>

#include "RvlCellAnim.hpp"

#include "../common.hpp"

#include "imgui.h"

class Animatable {
private:
    bool animating{ false };

    RvlCellAnim::Animation* currentAnimation{ nullptr };
    RvlCellAnim::AnimationKey* currentKey{ nullptr };

    int16_t holdKey{ 0 };

    uint16_t currentAnimationIndex{ 0 };
    uint16_t currentKeyIndex{ 0 };

public:
    RvlCellAnim::RvlCellAnimObject* cellanim{ nullptr };
    Common::Image* texture{ nullptr };

    ImVec2 offset{ 512, 512 };

    float scaleX{ 1.0f };
    float scaleY{ 1.0f };

    bool visible{ true };

    Animatable(RvlCellAnim::RvlCellAnimObject* cellanim, Common::Image* texture) : cellanim(cellanim), texture(texture) {}

    ~Animatable() {};

    void setAnimationFromIndex(uint16_t animIndex);
    void setAnimationKeyFromIndex(uint16_t keyIndex);

    void setAnimationKeyFromPtr(RvlCellAnim::AnimationKey* key);

    void setAnimating(bool animating);
    bool getAnimating() const;

    uint16_t getCurrentAnimationIndex() const;
    uint16_t getCurrentKeyIndex() const;

    int16_t getHoldFramesLeft() const;

    RvlCellAnim::AnimationKey* getCurrentKey() const;
    RvlCellAnim::Animation* getCurrentAnimation() const;
    RvlCellAnim::Arrangement* getCurrentArrangement() const;

    void Update();

    void Draw(ImDrawList* drawList, bool allowOpacity = true);

    // Does the Animatable actually draw anything at all to the screen?
    bool getDoesDraw(bool allowOpacity = true) const;

    ImVec2 getPartWorldSpace(RvlCellAnim::AnimationKey* key, uint16_t partIndex) const;
    std::array<ImVec2, 4> getPartWorldQuad(RvlCellAnim::AnimationKey* key, uint16_t partIndex) const;
};

#endif // ANIMATABLE_HPP