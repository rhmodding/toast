#ifndef ANIMATABLE_HPP
#define ANIMATABLE_HPP

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>

#include <memory>

#include "RvlCellAnim.hpp"

#include "../common.hpp"

class Animatable {
private:
    bool animating{ false };

    RvlCellAnim::Animation* currentAnimation{ nullptr };
    RvlCellAnim::AnimationKey* currentKey{ nullptr };

    int32_t holdKey{ 0 };

    uint16_t currentAnimationIndex{ 0 };
    uint16_t currentKeyIndex{ 0 };

    void DrawKey(
        RvlCellAnim::AnimationKey* key,
        ImDrawList* drawList,

        int32_t partIndex = -1,
        uint32_t colorMod = 0xFFFFFFFFu,
        bool allowOpacity = true
    ) __attribute__((nonnull));

public:
    std::shared_ptr<RvlCellAnim::RvlCellAnimObject> cellanim;
    std::shared_ptr<Common::Image> texture;

    ImVec2 offset{ CANVAS_ORIGIN, CANVAS_ORIGIN };

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

    void overrideAnimationKey(RvlCellAnim::AnimationKey* key)
        __attribute__((nonnull));

    void setAnimating(bool animating);
    bool getAnimating() const;

    uint16_t getCurrentAnimationIndex() const;
    uint16_t getCurrentKeyIndex() const;

    int32_t getHoldFramesLeft() const;

    RvlCellAnim::AnimationKey* getCurrentKey() const;
    RvlCellAnim::Animation* getCurrentAnimation() const;
    RvlCellAnim::Arrangement* getCurrentArrangement() const;

    // animation and key are pointers to a vector elements which can be remapped,
    // so when one of the vectors is modified this must be called.
    void refreshPointers();

    void Update();

    void Draw(ImDrawList* drawList, bool allowOpacity = true)
        __attribute__((nonnull));

    void DrawOnionSkin(ImDrawList* drawList, uint16_t backCount, uint16_t frontCount, uint8_t opacity)
        __attribute__((nonnull));

    // Does the Animatable actually draw anything at all to the screen?
    bool getDoesDraw(bool allowOpacity = true) const;

    std::array<ImVec2, 4> getPartWorldQuad(
        RvlCellAnim::AnimationKey* key, uint16_t partIndex
    ) const __attribute__((nonnull));
    ImRect getKeyWorldRect(RvlCellAnim::AnimationKey* key)
        const __attribute__((nonnull));
};

#endif // ANIMATABLE_HPP
