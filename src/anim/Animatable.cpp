#include "Animatable.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#include <array>

#include "../common.hpp"

ImVec2 rotateVec2(ImVec2 v, float angle, ImVec2 origin) {
    const float s = sinf(angle * ((float)M_PI / 180.f));
    const float c = cosf(angle * ((float)M_PI / 180.f));

    v.x -= origin.x;
    v.y -= origin.y;

    float x = v.x * c - v.y * s;
    float y = v.x * s + v.y * c;

    return { x + origin.x, y + origin.y };
}


void Animatable::setAnimationFromIndex(uint16_t animIndex) {
    SAFE_ASSERT(this->cellanim, true);
    SAFE_ASSERT(this->cellanim->animations.size() >= animIndex, true);

    this->currentAnimation = &this->cellanim->animations.at(animIndex);
    this->currentAnimationIndex = animIndex;

    this->currentKey = &this->currentAnimation->keys.at(0);
    this->currentKeyIndex = 0;

    this->holdKey = this->currentKey->holdFrames;
}

void Animatable::setAnimationKeyFromIndex(uint16_t keyIndex) {
    SAFE_ASSERT(this->cellanim, true);
    SAFE_ASSERT(this->currentAnimation, true);

    SAFE_ASSERT(this->currentAnimation->keys.size() >= keyIndex, true);

    this->currentKey = &this->currentAnimation->keys.at(keyIndex);
    this->currentKeyIndex = keyIndex;
    this->holdKey = this->currentKey->holdFrames;
}

void Animatable::setAnimating(bool animating) {
    this->animating = animating;
}
bool Animatable::getAnimating() const {
    return this->animating;
}

void Animatable::Update() {
    if (!this->animating)
        return;

    if (this->holdKey < 1) {
        if (this->currentKeyIndex + 1 >= this->currentAnimation->keys.size()) {
            this->animating = false;
            return;
        }

        this->currentKeyIndex++;
        this->currentKey = &this->currentAnimation->keys[currentKeyIndex];

        this->holdKey = this->currentKey->holdFrames;
    }

    this->holdKey--;
}

uint16_t Animatable::getCurrentAnimationIndex() const {
    return this->currentAnimationIndex;
}
uint16_t Animatable::getCurrentKeyIndex() const {
    return this->currentKeyIndex;
}

RvlCellAnim::AnimationKey* Animatable::getCurrentKey() const {
    return this->currentKey;
}

RvlCellAnim::Animation* Animatable::getCurrentAnimation() const {
    return this->currentAnimation;
}

RvlCellAnim::Arrangement* Animatable::getCurrentArrangement() const {
    return &this->cellanim->arrangements.at(this->getCurrentKey()->arrangementIndex);
}

void Animatable::refreshPointers() {
    this->currentAnimation = &this->cellanim->animations.at(this->currentAnimationIndex);
    this->currentKey = &this->currentAnimation->keys.at(this->currentKeyIndex);
}

int32_t Animatable::getHoldFramesLeft() const {
    return this->holdKey;
}

ImRect Animatable::getKeyWorldRect(RvlCellAnim::AnimationKey* key) const {
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

    return ImRect{{minX, minY}, {maxX, maxY}};
}

// Top-left, top-right, bottom-right, bottom-left
std::array<ImVec2, 4> Animatable::getPartWorldQuad(const RvlCellAnim::AnimationKey* key, uint16_t partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    SAFE_ASSERT_RET(this->cellanim, transformedQuad);

    const RvlCellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key->arrangementIndex);
    const RvlCellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const int*   arrngOffset = arrangement.tempOffset;
    const float* arrngScale  = arrangement.tempScale;

    ImVec2 keyCenter = {
        this->offset.x,
        this->offset.y
    };

    ImVec2 topLeftOffset = {
        (static_cast<float>(part.transform.positionX) - CANVAS_ORIGIN),
        (static_cast<float>(part.transform.positionY) - CANVAS_ORIGIN)
    };

    ImVec2 bottomRightOffset = {
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

    ImVec2 center = AVERAGE_IMVEC2(topLeftOffset, bottomRightOffset);

    // Transformations
    {
        // Flip operations
        if (part.flipX) {
            std::swap(transformedQuad[0], transformedQuad[1]);
            std::swap(transformedQuad[2], transformedQuad[3]);
        }
        if (part.flipY) {
            std::swap(transformedQuad[0], transformedQuad[3]);
            std::swap(transformedQuad[1], transformedQuad[2]);
        }

        // Rotation
        if (fmod(part.transform.angle, 360)) {
            float angle = part.transform.angle;

            if (arrngScale[0] < 0.f) angle = -angle;
            if (arrngScale[1] < 0.f) angle = -angle;

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
    SAFE_ASSERT_RET(this->cellanim, false);
    SAFE_ASSERT_RET(this->currentAnimation, false);

    if (!this->visible)
        return false;

    RvlCellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(this->currentKey->arrangementIndex);

    for (auto part : arrangement.parts) {
        if (allowOpacity) {
            if (part.opacity > 0)
                return true;
        }
        else
            return true;
    }

    return false;
}

void Animatable::overrideAnimationKey(RvlCellAnim::AnimationKey* key) {
    SAFE_ASSERT(this->cellanim, true);
    SAFE_ASSERT(key, true);

    this->animating = false;
    this->currentKey = key;
}

void Animatable::DrawKey(
    const RvlCellAnim::AnimationKey* key,
    ImDrawList* drawList,

    int partIndex,
    unsigned colorMod,
    bool allowOpacity
) {
    SAFE_ASSERT(this->cellanim, true);
    SAFE_ASSERT(this->currentAnimation, true);

    const RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(key->arrangementIndex);

    const float mismatchScaleX = static_cast<float>(this->texture->width) / this->cellanim->textureW;
    const float mismatchScaleY = static_cast<float>(this->texture->height) / this->cellanim->textureH;

    for (unsigned i = 0; i < arrangement->parts.size(); i++) {
        if (partIndex != -1 && partIndex != i)
            continue;

        const RvlCellAnim::ArrangementPart& part = arrangement->parts.at(i);

        // Skip invisible parts
        if (((part.opacity == 0) && allowOpacity) || !part.editorVisible)
            continue;

        const uint16_t sourceRect[4] = {
            static_cast<uint16_t>(part.regionX * mismatchScaleX),
            static_cast<uint16_t>(part.regionY * mismatchScaleY),
            static_cast<uint16_t>(part.regionW * mismatchScaleX),
            static_cast<uint16_t>(part.regionH * mismatchScaleY)
        };

        auto transformedQuad = this->getPartWorldQuad(key, i);

        ImVec2 uvTopLeft = {
            sourceRect[0] / static_cast<float>(this->texture->width),
            sourceRect[1] / static_cast<float>(this->texture->height)
        };
        ImVec2 uvBottomRight = {
            uvTopLeft.x + (sourceRect[2] / static_cast<float>(this->texture->width)),
            uvTopLeft.y + (sourceRect[3] / static_cast<float>(this->texture->height))
        };

        ImVec2 uvs[4] = {
            uvTopLeft,
            { uvBottomRight.x, uvTopLeft.y },
            uvBottomRight,
            { uvTopLeft.x, uvBottomRight.y }
        };

        uint8_t modAlpha = (colorMod >> 24) & 0xFF;
        uint8_t baseAlpha = allowOpacity ? 
            static_cast<uint8_t>((static_cast<uint16_t>(part.opacity) * key->opacity) / 255) : 255;

        drawList->AddImageQuad(
            (void*)(intptr_t)this->texture->texture,
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            uvs[0], uvs[1], uvs[2], uvs[3],
            IM_COL32(
                (colorMod >>  0) & 0xFF,
                (colorMod >>  8) & 0xFF,
                (colorMod >> 16) & 0xFF,
                static_cast<uint8_t>((baseAlpha * static_cast<uint16_t>(modAlpha)) / 255)
            )
        );
    }
}

void Animatable::Draw(ImDrawList* drawList, bool allowOpacity) {
    SAFE_ASSERT(this->cellanim, true);
    SAFE_ASSERT(this->currentAnimation, true);

    if (!this->visible)
        return;

    this->DrawKey(this->currentKey, drawList, -1, 0xFFFFFFFFu, allowOpacity);
}

void Animatable::DrawOnionSkin(ImDrawList* drawList, uint16_t backCount, uint16_t frontCount, uint8_t opacity) {
    SAFE_ASSERT(this->cellanim, true);
    SAFE_ASSERT(this->currentAnimation, true);

    if (!this->visible || !drawList || !this->currentAnimation)
        return;

    const int keyIndex = this->currentKeyIndex;

    auto _drawOnionSkin = [&](int startIndex, int endIndex, int step, ImU32 color) {
        for (int i = startIndex; (step < 0) ? (i >= endIndex) : (i <= endIndex); i += step) {
            if (i < 0 || i >= this->currentAnimation->keys.size())
                break;

            this->DrawKey(&this->currentAnimation->keys.at(i), drawList, -1, color);
        }
    };

    if (backCount > 0)
        _drawOnionSkin(keyIndex - 1, keyIndex - backCount, -1, IM_COL32(255, 64, 64, opacity));

    if (frontCount > 0)
        _drawOnionSkin(keyIndex + 1, keyIndex + frontCount, 1, IM_COL32(64, 255, 64, opacity));
}
