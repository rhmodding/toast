#ifndef CELLANIMRENDERER_HPP
#define CELLANIMRENDERER_HPP

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>

#include <memory>

#include "CellAnim.hpp"

#include "../texture/TextureGroup.hpp"

class CellAnimRenderer {
public:
    static void InitShader();
    static void DestroyShader();

private:
    static void renderPartCallback(const ImDrawList* parentList, const ImDrawCmd* cmd);

    static GLuint shaderProgram;

    static GLint textureUniform;
    static GLint foreColorAUniform;
    static GLint backColorAUniform;
    static GLint foreColorBUniform;
    static GLint backColorBUniform;
    static GLint projMtxUniform;

    static GLint positionAttrib;
    static GLint uvAttrib;
    static GLint colorAttrib;

    static GLuint texDrawFramebuffer;

public:
    CellAnimRenderer() = default;
    ~CellAnimRenderer() = default;

    void linkCellAnim(std::shared_ptr<CellAnim::CellAnimObject> cellAnim) {
        mCellAnim = cellAnim;
    }
    void linkTextureGroup(std::shared_ptr<TextureGroup> textureGroup) {
        mTextureGroup = textureGroup;
    }

    ImVec2 getOffset() const { return mOffset; }
    void setOffset(ImVec2 offset) { mOffset = offset; }

    ImVec2 getScale() const { return mScale; }
    void setScale(ImVec2 scale) { mScale = scale; }

    bool getVisible() const { return mVisible; }
    void setVisible(bool visible) { mVisible = visible; }

    void Draw(
        ImDrawList* drawList,
        const CellAnim::Animation& animation, unsigned keyIndex,
        bool allowOpacity = true
    ) __attribute__((nonnull));

    void DrawTex(
        GLuint textureId,
        const CellAnim::Animation& animation, unsigned keyIndex,
        bool allowOpacity = true
    );

    void DrawOnionSkin(
        ImDrawList* drawList,
        const CellAnim::Animation& animation, unsigned keyIndex,
        unsigned backCount, unsigned frontCount,
        bool rollOver,
        uint8_t opacity
    ) __attribute__((nonnull));

    std::array<ImVec2, 4> getPartWorldQuad(const CellAnim::AnimationKey& key, unsigned partIndex) const;
    std::array<ImVec2, 4> getPartWorldQuad(const CellAnim::TransformValues& keyTransform, const CellAnim::Arrangement& arrangement, unsigned partIndex) const;
    ImRect getKeyWorldRect(const CellAnim::AnimationKey& key) const;

private:
    enum class DrawMethod {
        DrawList,
        Texture
    };

    ImDrawList* currentDrawList;
    GLuint currentDrawTex;

    void InternDraw(
        DrawMethod drawMethod,
        const CellAnim::AnimationKey& key, int partIndex,
        uint32_t colorMod,
        bool allowOpacity
    );

private:
    std::shared_ptr<CellAnim::CellAnimObject> mCellAnim;
    std::shared_ptr<TextureGroup> mTextureGroup;

    ImVec2 mOffset { 0.f, 0.f };
    ImVec2 mScale { 1.f, 1.f };

    bool mVisible { true };

public: // TODO: filthy HACK ..
    // These get set by InternDraw.
    ImVec2 mDrawTexSize;
    ImVec2 mDrawTexOffset;
};

#endif // CELLANIMRENDERER_HPP
