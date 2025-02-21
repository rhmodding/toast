#ifndef CELLANIMRENDERER_HPP
#define CELLANIMRENDERER_HPP

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>

#include <memory>

#include "RvlCellAnim.hpp"

#include "../texture/Texture.hpp"
#include "../texture/TextureGroup.hpp"

class CellanimRenderer {
public:
    CellanimRenderer() = default;
    ~CellanimRenderer() = default;

    void linkCellanim(std::shared_ptr<RvlCellAnim::RvlCellAnimObject> cellanim_) {
        this->cellanim = cellanim_;
    }
    void linkTextureGroup(std::shared_ptr<TextureGroup> textureGroup_) {
        this->textureGroup = textureGroup_;
    }

    void Draw(
        ImDrawList* drawList,
        const RvlCellAnim::Animation& animation, unsigned keyIndex,
        bool allowOpacity = true
    ) __attribute__((nonnull));

    void DrawOnionSkin(
        ImDrawList* drawList,
        const RvlCellAnim::Animation& animation, unsigned keyIndex,
        unsigned backCount, unsigned frontCount,
        bool rollOver,
        uint8_t opacity
    ) __attribute__((nonnull));

    std::array<ImVec2, 4> getPartWorldQuad(const RvlCellAnim::AnimationKey& key, unsigned partIndex) const;
    std::array<ImVec2, 4> getPartWorldQuad(const RvlCellAnim::TransformValues keyTransform, const RvlCellAnim::Arrangement& arrangement, unsigned partIndex) const;
    ImRect getKeyWorldRect(const RvlCellAnim::AnimationKey& key) const;

private:
    void InternDraw(
        ImDrawList* drawList,
        const RvlCellAnim::AnimationKey& key, int partIndex,
        uint32_t colorMod,
        bool allowOpacity
    );

public:
    ImVec2 offset { 0.f, 0.f };

    float scaleX { 1.f };
    float scaleY { 1.f };

    bool visible { true };

private:
    std::shared_ptr<RvlCellAnim::RvlCellAnimObject> cellanim;
    std::shared_ptr<TextureGroup> textureGroup;
};

#endif // CELLANIMRENDERER_HPP
