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

    int holdKey{ 0 };

    unsigned currentAnimationIndex{ 0 };
    unsigned currentKeyIndex{ 0 };

    void DrawKey(
        const RvlCellAnim::AnimationKey* key,
        ImDrawList* drawList,

        int partIndex = -1,
        unsigned colorMod = 0xFFFFFFFFu,
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

    void setAnimationFromIndex(unsigned animIndex);
    void setAnimationKeyFromIndex(unsigned keyIndex);

    void overrideAnimationKey(RvlCellAnim::AnimationKey* key)
        __attribute__((nonnull));

    void setAnimating(bool animating);
    bool getAnimating() const;

    unsigned getCurrentAnimationIndex() const;
    unsigned getCurrentKeyIndex() const;

    int getHoldFramesLeft() const;

    RvlCellAnim::AnimationKey* getCurrentKey() const;
    RvlCellAnim::Animation* getCurrentAnimation() const;
    RvlCellAnim::Arrangement* getCurrentArrangement() const;

    // animation and key are pointers to a vector elements which can be remapped,
    // so when one of the vectors is modified this must be called.
    void refreshPointers();

    void Update();

    void Draw(ImDrawList* drawList, bool allowOpacity = true)
        __attribute__((nonnull));

    void DrawOnionSkin(ImDrawList* drawList, unsigned backCount, unsigned frontCount, bool rollOver, uint8_t opacity)
        __attribute__((nonnull));

    // Does the Animatable actually draw anything at all to the screen?
    bool getDoesDraw(bool allowOpacity = true) const;

    std::array<ImVec2, 4> getPartWorldQuad(
        const RvlCellAnim::AnimationKey* key, unsigned partIndex
    ) const __attribute__((nonnull));
    ImRect getKeyWorldRect(RvlCellAnim::AnimationKey* key)
        const __attribute__((nonnull));
};

#endif // ANIMATABLE_HPP
