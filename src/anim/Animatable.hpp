#ifndef ANIMATABLE_HPP
#define ANIMATABLE_HPP

#include <cstdint>

#include "RvlCellAnim.hpp"

#include "../common.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <memory>

class Animatable {
private:
    bool animating{ false };

    RvlCellAnim::Animation* currentAnimation{ nullptr };
    RvlCellAnim::AnimationKey* currentKey{ nullptr };

    int16_t holdKey{ 0 };

    uint16_t currentAnimationIndex{ 0 };
    uint16_t currentKeyIndex{ 0 };

    void DrawKey(
        RvlCellAnim::AnimationKey* key,
        ImDrawList* drawList,

        int32_t partIndex = -1,
        uint32_t colorMod = 0xFFFFFFFFu,
        bool allowOpacity = true
    );

public:
    std::shared_ptr<RvlCellAnim::RvlCellAnimObject> cellanim;
    std::shared_ptr<Common::Image> texture;

    ImVec2 offset{ 512, 512 };

    float scaleX{ 1.f };
    float scaleY{ 1.f };

    bool visible{ true };

    Animatable(
        std::shared_ptr<RvlCellAnim::RvlCellAnimObject> cellanim,
        std::shared_ptr<Common::Image> texture
    ) :
        cellanim(cellanim), texture(texture)
    {}

    ~Animatable() {};

    void setAnimationFromIndex(uint16_t animIndex);
    void setAnimationKeyFromIndex(uint16_t keyIndex);

    void overrideAnimationKey(RvlCellAnim::AnimationKey* key);

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

    void DrawOnionSkin(ImDrawList* drawList, uint16_t backCount, uint16_t frontCount, uint8_t opacity);

    // Does the Animatable actually draw anything at all to the screen?
    bool getDoesDraw(bool allowOpacity = true) const;

    std::array<ImVec2, 4> getPartWorldQuad(RvlCellAnim::AnimationKey* key, uint16_t partIndex) const;
    ImRect getKeyWorldRect(RvlCellAnim::AnimationKey* key) const;
};

#endif // ANIMATABLE_HPP