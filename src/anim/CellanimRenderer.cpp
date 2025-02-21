#include "CellanimRenderer.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#include <array>

#include <algorithm>

#include "../common.hpp"

void CellanimRenderer::Draw(ImDrawList* drawList, const CellAnim::Animation& animation, unsigned keyIndex, bool allowOpacity) {
    NONFATAL_ASSERT_RET(this->cellanim, true);
    NONFATAL_ASSERT_RET(this->textureGroup, true);

    if (!this->visible)
        return;

    this->InternDraw(drawList, animation.keys.at(keyIndex), -1, 0xFFFFFFFFu, allowOpacity);
}

void CellanimRenderer::DrawOnionSkin(ImDrawList* drawList, const CellAnim::Animation& animation, unsigned keyIndex, unsigned backCount, unsigned frontCount, bool rollOver, uint8_t opacity) {
    NONFATAL_ASSERT_RET(this->cellanim, true);
    NONFATAL_ASSERT_RET(this->textureGroup, true);

    if (!this->visible)
        return;

    const unsigned keyCount = animation.keys.size();

    auto _drawOnionSkin = [&](int startIndex, int endIndex, int step, uint32_t color) {
        int i = startIndex;
        while ((step < 0) ? (i >= endIndex) : (i <= endIndex)) {
            int wrappedIndex = i;

            if (rollOver) {
                // Roll backwards
                if (wrappedIndex < 0)
                    wrappedIndex = (keyCount + (wrappedIndex % keyCount)) % keyCount;
                // Roll forwards
                else if (wrappedIndex >= (int)keyCount)
                    wrappedIndex = wrappedIndex % keyCount;
            }

            // Break out if rollover is disabled
            if (!rollOver && (wrappedIndex < 0 || wrappedIndex >= (int)keyCount))
                break;

            this->InternDraw(drawList, animation.keys.at(wrappedIndex), -1, color, true);
            i += step;
        }
    };

    if (backCount > 0)
        _drawOnionSkin(keyIndex - 1, keyIndex - backCount, -1, IM_COL32(255, 64, 64, opacity));

    if (frontCount > 0)
        _drawOnionSkin(keyIndex + 1, keyIndex + frontCount, 1, IM_COL32(64, 255, 64, opacity));
}

// Note: 'angle' is in degrees.
static ImVec2 rotateVec2(const ImVec2& v, float angle, const ImVec2& origin) {
    const float s = sinf(angle * ((float)M_PI / 180.f));
    const float c = cosf(angle * ((float)M_PI / 180.f));

    float vx = v.x - origin.x;
    float vy = v.y - origin.y;

    float x = vx * c - vy * s;
    float y = vx * s + vy * c;

    return { x + origin.x, y + origin.y };
}

std::array<ImVec2, 4> CellanimRenderer::getPartWorldQuad(const CellAnim::AnimationKey& key, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(this->cellanim, transformedQuad);

    const CellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key.arrangementIndex);
    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const int*   tempOffset = arrangement.tempOffset;
    const float* tempScale  = arrangement.tempScale;

    ImVec2 keyCenter {
        this->offset.x,
        this->offset.y
    };

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.positionX),
        static_cast<float>(part.transform.positionY)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionW * part.transform.scaleX)),
        (topLeftOffset.y + (part.regionH * part.transform.scaleY))
    };

    topLeftOffset = {
        (topLeftOffset.x * tempScale[0]) + tempOffset[0],
        (topLeftOffset.y * tempScale[1]) + tempOffset[1]
    };
    bottomRightOffset = {
        (bottomRightOffset.x * tempScale[0]) + tempOffset[0],
        (bottomRightOffset.y * tempScale[1]) + tempOffset[1]
    };

    transformedQuad = {
        topLeftOffset,
        { bottomRightOffset.x, topLeftOffset.y },
        bottomRightOffset,
        { topLeftOffset.x, bottomRightOffset.y },
    };

    const ImVec2 center = AVERAGE_IMVEC2(topLeftOffset, bottomRightOffset);

    // Transformations
    {
        // Rotation
        float rotAngle = part.transform.angle;

        if ((tempScale[0] < 0.f) ^ (tempScale[1] < 0.f))
            rotAngle = -rotAngle;

        for (auto& point : transformedQuad)
            point = rotateVec2(point, rotAngle, center);

        // Key & renderer scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * key.transform.scaleX * this->scaleX) + keyCenter.x;
            point.y = (point.y * key.transform.scaleY * this->scaleY) + keyCenter.y;
        }

        // Key rotation
        for (auto& point : transformedQuad)
            point = rotateVec2(point, key.transform.angle, keyCenter);

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += key.transform.positionX * this->scaleX;
            point.y += key.transform.positionY * this->scaleY;
        }
    }

    return transformedQuad;
}

std::array<ImVec2, 4> CellanimRenderer::getPartWorldQuad(const CellAnim::TransformValues keyTransform, const CellAnim::Arrangement& arrangement, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(this->cellanim, transformedQuad);

    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const int*   tempOffset = arrangement.tempOffset;
    const float* tempScale  = arrangement.tempScale;

    ImVec2 keyCenter {
        this->offset.x,
        this->offset.y
    };

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.positionX),
        static_cast<float>(part.transform.positionY)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionW * part.transform.scaleX)),
        (topLeftOffset.y + (part.regionH * part.transform.scaleY))
    };

    topLeftOffset = {
        (topLeftOffset.x * tempScale[0]) + tempOffset[0],
        (topLeftOffset.y * tempScale[1]) + tempOffset[1]
    };
    bottomRightOffset = {
        (bottomRightOffset.x * tempScale[0]) + tempOffset[0],
        (bottomRightOffset.y * tempScale[1]) + tempOffset[1]
    };

    transformedQuad = {
        topLeftOffset,
        { bottomRightOffset.x, topLeftOffset.y },
        bottomRightOffset,
        { topLeftOffset.x, bottomRightOffset.y },
    };

    const ImVec2 center = AVERAGE_IMVEC2(topLeftOffset, bottomRightOffset);

    // Transformations
    {
        // Rotation
        float rotAngle = part.transform.angle;

        if ((tempScale[0] < 0.f) ^ (tempScale[1] < 0.f))
            rotAngle = -rotAngle;

        for (auto& point : transformedQuad)
            point = rotateVec2(point, rotAngle, center);

        // Key & renderer scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * keyTransform.scaleX * this->scaleX) + keyCenter.x;
            point.y = (point.y * keyTransform.scaleY * this->scaleY) + keyCenter.y;
        }

        // Key rotation
        for (auto& point : transformedQuad)
            point = rotateVec2(point, keyTransform.angle, keyCenter);

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += keyTransform.positionX * this->scaleX;
            point.y += keyTransform.positionY * this->scaleY;
        }
    }

    return transformedQuad;
}

ImRect CellanimRenderer::getKeyWorldRect(const CellAnim::AnimationKey& key) const {
    NONFATAL_ASSERT_RETVAL(this->cellanim, ImRect());

    const auto& arrangement = this->cellanim->arrangements.at(key.arrangementIndex);

    std::vector<std::array<ImVec2, 4>> quads(arrangement.parts.size());
    for (unsigned i = 0; i < arrangement.parts.size(); i++)
        quads[i] = this->getPartWorldQuad(key, i);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (unsigned i = 0; i < arrangement.parts.size(); ++i) {
        for (const ImVec2& vertex : quads[i]) {
            if (vertex.x < minX) minX = vertex.x;
            if (vertex.y < minY) minY = vertex.y;
            if (vertex.x > maxX) maxX = vertex.x;
            if (vertex.y > maxY) maxY = vertex.y;
        }
    }

    return ImRect({ minX, minY }, { maxX, maxY });
}

void CellanimRenderer::InternDraw(
    ImDrawList* drawList,
    const CellAnim::AnimationKey& key, int partIndex,
    uint32_t colorMod,
    bool allowOpacity
) {
    NONFATAL_ASSERT_RET(this->cellanim, true);

    const CellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key.arrangementIndex);

    const float texWidth = this->cellanim->sheetW;
    const float texHeight = this->cellanim->sheetH;

    for (unsigned i = 0; i < arrangement.parts.size(); i++) {
        if (partIndex != -1 && partIndex != (int)i)
            continue;

        const CellAnim::ArrangementPart& part = arrangement.parts.at(i);

        // Skip invisible parts
        if (((part.opacity == 0) && allowOpacity) || !part.editorVisible)
            continue;

        auto transformedQuad = this->getPartWorldQuad(key, i);

        ImVec2 uvTopLeft = {
            part.regionX / texWidth,
            part.regionY / texHeight
        };
        ImVec2 uvBottomRight = {
            uvTopLeft.x + (part.regionW / texWidth),
            uvTopLeft.y + (part.regionH / texHeight)
        };

        ImVec2 uvs[4] = {
            uvTopLeft,
            { uvBottomRight.x, uvTopLeft.y },
            uvBottomRight,
            { uvTopLeft.x, uvBottomRight.y }
        };

        if (part.flipX) {
            std::swap(uvs[0], uvs[1]);
            std::swap(uvs[2], uvs[3]);
        }
        if (part.flipY) {
            std::swap(uvs[0], uvs[3]);
            std::swap(uvs[1], uvs[2]);
        }

        uint8_t modAlpha = (colorMod >> 24) & 0xFF;
        uint8_t baseAlpha = allowOpacity ?
            uint8_t((unsigned(part.opacity) * unsigned(key.opacity)) / 0xFFu) :
            0xFFu;

        const auto& texture = this->textureGroup->getTextureByVarying(part.textureVarying);

        drawList->AddImageQuad(
            (ImTextureID)texture->getTextureId(),
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            uvs[0], uvs[1], uvs[2], uvs[3],
            IM_COL32(
                (colorMod >>  0) & 0xFF,
                (colorMod >>  8) & 0xFF,
                (colorMod >> 16) & 0xFF,
                uint8_t((unsigned(baseAlpha) * unsigned(modAlpha)) / 0xFFu)
            )
        );
    }
}

/*
// In degrees.
static ImVec2 rotateVec2(const ImVec2& v, float angle, const ImVec2& origin) {
    const float s = sinf(angle * ((float)M_PI / 180.f));
    const float c = cosf(angle * ((float)M_PI / 180.f));

    float vx = v.x - origin.x;
    float vy = v.y - origin.y;

    float x = vx * c - vy * s;
    float y = vx * s + vy * c;

    return { x + origin.x, y + origin.y };
}

void Animatable::setAnimationFromIndex(unsigned animIndex) {
    NONFATAL_ASSERT_RET(this->cellanim, true);
    NONFATAL_ASSERT_RET(this->cellanim->animations.size() >= animIndex, true);

    this->currentAnimationIndex = animIndex;
    this->currentKeyIndex = 0;

    this->holdKey = this->getCurrentKey()->holdFrames;

    this->overrideKey = nullptr;
}

void Animatable::setAnimationKeyFromIndex(unsigned keyIndex) {
    NONFATAL_ASSERT_RET(this->cellanim, true);

    NONFATAL_ASSERT_RET(this->getCurrentAnimation()->keys.size() >= keyIndex, true);

    this->currentKeyIndex = keyIndex;
    this->holdKey = this->getCurrentKey()->holdFrames;

    this->overrideKey = nullptr;
}

void Animatable::setAnimating(bool _animating) {
    this->animating = _animating;
}
bool Animatable::getAnimating() const {
    return this->animating;
}

void Animatable::Update() {
    if (!this->animating)
        return;

    if (this->holdKey < 1) {
        if (this->currentKeyIndex + 1 >= this->getCurrentAnimation()->keys.size()) {
            this->animating = false;
            return;
        }

        this->currentKeyIndex++;
        this->holdKey = this->getCurrentKey()->holdFrames;
    }

    this->holdKey--;
}

unsigned Animatable::getCurrentAnimationIndex() const {
    return this->currentAnimationIndex;
}
unsigned Animatable::getCurrentKeyIndex() const {
    return this->currentKeyIndex;
}

CellAnim::AnimationKey* Animatable::getCurrentKey() const {
    if (this->overrideKey)
        return this->overrideKey;
    return &this->cellanim->animations.at(this->currentAnimationIndex).keys.at(this->currentKeyIndex);
}

CellAnim::Animation* Animatable::getCurrentAnimation() const {
    return &this->cellanim->animations.at(this->currentAnimationIndex);
}

CellAnim::Arrangement* Animatable::getCurrentArrangement() const {
    return &this->cellanim->arrangements.at(this->getCurrentKey()->arrangementIndex);
}

int Animatable::getHoldFramesLeft() const {
    return this->holdKey;
}

ImRect Animatable::getKeyWorldRect(CellAnim::AnimationKey* key) const {
    const auto& arrangement = this->cellanim->arrangements.at(key->arrangementIndex);

    std::vector<std::array<ImVec2, 4>> quads(arrangement.parts.size());
    for (unsigned i = 0; i < arrangement.parts.size(); i++)
        quads[i] = this->getPartWorldQuad(key, i);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (unsigned i = 0; i < arrangement.parts.size(); ++i) {
        for (const ImVec2& vertex : quads[i]) {
            if (vertex.x < minX) minX = vertex.x;
            if (vertex.y < minY) minY = vertex.y;
            if (vertex.x > maxX) maxX = vertex.x;
            if (vertex.y > maxY) maxY = vertex.y;
        }
    }

    return ImRect({ minX, minY }, { maxX, maxY });
}

// Top-left, top-right, bottom-right, bottom-left
std::array<ImVec2, 4> Animatable::getPartWorldQuad(const CellAnim::AnimationKey* key, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(this->cellanim, transformedQuad);

    const CellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key->arrangementIndex);
    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const int*   arrngOffset = arrangement.tempOffset;
    const float* arrngScale  = arrangement.tempScale;

    ImVec2 keyCenter {
        this->offset.x,
        this->offset.y
    };

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.positionX),
        static_cast<float>(part.transform.positionY)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionW * part.transform.scaleX)),
        (topLeftOffset.y + (part.regionH * part.transform.scaleY))
    };

    topLeftOffset = {
        (topLeftOffset.x * arrngScale[0]) + arrngOffset[0],
        (topLeftOffset.y * arrngScale[1]) + arrngOffset[1]
    };
    bottomRightOffset = {
        (bottomRightOffset.x * arrngScale[0]) + arrngOffset[0],
        (bottomRightOffset.y * arrngScale[1]) + arrngOffset[1]
    };

    transformedQuad = {
        topLeftOffset,
        { bottomRightOffset.x, topLeftOffset.y },
        bottomRightOffset,
        { topLeftOffset.x, bottomRightOffset.y },
    };

    const ImVec2 center = AVERAGE_IMVEC2(topLeftOffset, bottomRightOffset);

    // Transformations
    {
        // Rotation
        if (fmod(part.transform.angle, 360)) {
            float angle = part.transform.angle;

            if (arrngScale[0] < 0.f)
                angle = -angle;
            if (arrngScale[1] < 0.f)
                angle = -angle;

            for (auto& point : transformedQuad)
                point = rotateVec2(point, angle, center);
        }

        // Key & animatable scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * key->transform.scaleX * this->scaleX) + keyCenter.x;
            point.y = (point.y * key->transform.scaleY * this->scaleY) + keyCenter.y;
        }

        // Key rotation
        if (fmod(key->transform.angle, 360)) {
            for (auto& point : transformedQuad)
                point = rotateVec2(point, key->transform.angle, keyCenter);
        }

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += key->transform.positionX * this->scaleX;
            point.y += key->transform.positionY * this->scaleY;
        }
    }

    return transformedQuad;
}

bool Animatable::getDoesDraw(bool allowOpacity) const {
    NONFATAL_ASSERT_RETVAL(this->cellanim, false);

    if (!this->visible)
        return false;

    const CellAnim::Arrangement& arrangement =
        this->cellanim->arrangements.at(this->getCurrentKey()->arrangementIndex);

    if (!allowOpacity && !arrangement.parts.empty())
        return true;

    for (const auto& part : arrangement.parts) {
        if (part.opacity > 0)
            return true;
    }

    return false;
}

void Animatable::overrideAnimationKey(CellAnim::AnimationKey* key) {
    NONFATAL_ASSERT_RET(this->cellanim, true);
    NONFATAL_ASSERT_RET(key, true);

    this->animating = false;
    this->overrideKey = key;
}

void Animatable::DrawKey(
    const CellAnim::AnimationKey* key,
    ImDrawList* drawList,

    int partIndex,
    unsigned colorMod,
    bool allowOpacity
) {
    NONFATAL_ASSERT_RET(this->cellanim, true);

    const CellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key->arrangementIndex);

    const float texWidth = this->cellanim->sheetW;
    const float texHeight = this->cellanim->sheetH;

    for (unsigned i = 0; i < arrangement.parts.size(); i++) {
        if (partIndex != -1 && partIndex != (int)i)
            continue;

        const CellAnim::ArrangementPart& part = arrangement.parts.at(i);

        // Skip invisible parts
        if (((part.opacity == 0) && allowOpacity) || !part.editorVisible)
            continue;

        auto transformedQuad = this->getPartWorldQuad(key, i);

        ImVec2 uvTopLeft = {
            part.regionX / texWidth,
            part.regionY / texHeight
        };
        ImVec2 uvBottomRight = {
            uvTopLeft.x + (part.regionW / texWidth),
            uvTopLeft.y + (part.regionH / texHeight)
        };

        ImVec2 uvs[4] = {
            uvTopLeft,
            { uvBottomRight.x, uvTopLeft.y },
            uvBottomRight,
            { uvTopLeft.x, uvBottomRight.y }
        };

        if (part.flipX) {
            std::swap(uvs[0], uvs[1]);
            std::swap(uvs[2], uvs[3]);
        }
        if (part.flipY) {
            std::swap(uvs[0], uvs[3]);
            std::swap(uvs[1], uvs[2]);
        }

        uint8_t modAlpha = (colorMod >> 24) & 0xFF;
        uint8_t baseAlpha = allowOpacity ?
            uint8_t((unsigned(part.opacity) * unsigned(key->opacity)) / 0xFFu) :
            0xFFu;

        drawList->AddImageQuad(
            (ImTextureID)this->texture->getTextureId(),
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            uvs[0], uvs[1], uvs[2], uvs[3],
            IM_COL32(
                (colorMod >>  0) & 0xFF,
                (colorMod >>  8) & 0xFF,
                (colorMod >> 16) & 0xFF,
                uint8_t((unsigned(baseAlpha) * unsigned(modAlpha)) / 0xFFu)
            )
        );
    }
}

void Animatable::Draw(ImDrawList* drawList, bool allowOpacity) {
    NONFATAL_ASSERT_RET(this->cellanim, true);

    if (!this->visible)
        return;

    this->DrawKey(this->getCurrentKey(), drawList, -1, 0xFFFFFFFFu, allowOpacity);
}

void Animatable::DrawOnionSkin(ImDrawList* drawList, unsigned backCount, unsigned frontCount, bool rollOver, uint8_t opacity) {
    NONFATAL_ASSERT_RET(this->cellanim, true);

    if (!this->visible)
        return;

    auto* currentAnimation = this->getCurrentAnimation();

    const unsigned keyIndex = this->currentKeyIndex;
    const unsigned keyCount = currentAnimation->keys.size();

    auto _drawOnionSkin = [&](int startIndex, int endIndex, int step, uint32_t color) {
        int i = startIndex;
        while ((step < 0) ? (i >= endIndex) : (i <= endIndex)) {
            int wrappedIndex = i;

            if (rollOver) {
                // Roll backwards
                if (wrappedIndex < 0)
                    wrappedIndex = (keyCount + (wrappedIndex % keyCount)) % keyCount;
                // Roll forwards
                else if (wrappedIndex >= (int)keyCount)
                    wrappedIndex = wrappedIndex % keyCount;
            }

            // Break out if rollover is disabled
            if (!rollOver && (wrappedIndex < 0 || wrappedIndex >= (int)keyCount))
                break;

            this->DrawKey(&currentAnimation->keys.at(wrappedIndex), drawList, -1, color);
            i += step;
        }
    };

    if (backCount > 0)
        _drawOnionSkin(keyIndex - 1, keyIndex - backCount, -1, IM_COL32(255, 64, 64, opacity));

    if (frontCount > 0)
        _drawOnionSkin(keyIndex + 1, keyIndex + frontCount, 1, IM_COL32(64, 255, 64, opacity));
}

*/
