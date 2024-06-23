#include "Animatable.hpp"

#undef NDEBUG
#include <assert.h>

#include "../AppState.hpp"

#include "../common.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

#include <array>

#define CANVAS_ORIGIN 512

void Animatable::setAnimationFromIndex(uint16_t animIndex) {
    assert(this->cellanim);

    assert(this->cellanim->animations.size() >= animIndex);

    this->currentAnimation = &this->cellanim->animations.at(animIndex);
    this->currentAnimationIndex = animIndex;

    this->currentKey = &this->currentAnimation->keys.at(0);
    this->currentKeyIndex = 0;

    this->holdKey = this->currentKey->holdFrames;
}

void Animatable::setAnimationKeyFromIndex(uint16_t keyIndex) {
    assert(this->cellanim);
    assert(this->currentAnimation);

    assert(this->currentAnimation->keys.size() >= keyIndex);

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
        if (
            static_cast<uint32_t>(this->currentKeyIndex + 1) >=
            static_cast<uint32_t>(this->currentAnimation->keys.size())
        ) {
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

int16_t Animatable::getHoldFramesLeft() const {
    return this->holdKey;
}

ImVec2 Animatable::getPartWorldSpace(RvlCellAnim::AnimationKey* key, uint16_t partIndex) const {
    assert(this->cellanim);

    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(key->arrangementIndex);
    RvlCellAnim::ArrangementPart part = arrangement->parts.at(partIndex);

    {
        float totalScaleX = key->scaleX * this->scaleX;
        float totalScaleY = key->scaleY * this->scaleY;

        ImVec2 point = {
            CANVAS_ORIGIN + ((part.positionX - CANVAS_ORIGIN) * totalScaleX) + (this->offset.x - CANVAS_ORIGIN),
            CANVAS_ORIGIN + ((part.positionY - CANVAS_ORIGIN) * totalScaleY) + (this->offset.y - CANVAS_ORIGIN),
        };

        return point;
    }
}

ImVec2 rotateVec2(ImVec2 v, float angle, ImVec2 origin) {
    float s = sinf(angle * ((float)M_PI / 180.f));
    float c = cosf(angle * ((float)M_PI / 180.f));

    v.x -= origin.x;
    v.y -= origin.y;

    float x = v.x * c - v.y * s;
    float y = v.x * s + v.y * c;

    return { x + origin.x, y + origin.y };
}

std::array<ImVec2, 4> Animatable::getPartWorldQuad(RvlCellAnim::AnimationKey* key, uint16_t partIndex) const {
    assert(this->cellanim);

    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(key->arrangementIndex);
    RvlCellAnim::ArrangementPart part = arrangement->parts.at(partIndex);

    float mismatchScaleX = static_cast<float>(this->texture->width) / this->cellanim->textureW;
    float mismatchScaleY = static_cast<float>(this->texture->height) / this->cellanim->textureH;

    uint16_t sourceRect[4] = {
        static_cast<uint16_t>(part.regionX * mismatchScaleX),
        static_cast<uint16_t>(part.regionY * mismatchScaleY),
        static_cast<uint16_t>(part.regionW * mismatchScaleX),
        static_cast<uint16_t>(part.regionH * mismatchScaleY)
    };

    // float totalScaleX = this->currentKey->scaleX * this->scaleX;
    // float totalScaleY = this->currentKey->scaleY * this->scaleY;

    ImVec2 keyCenter = {
        CANVAS_ORIGIN + (this->offset.x - CANVAS_ORIGIN),
        CANVAS_ORIGIN + (this->offset.y - CANVAS_ORIGIN)
    };

    ImVec2 topLeftOffset = {
        static_cast<float>(part.positionX) - CANVAS_ORIGIN,
        static_cast<float>(part.positionY) - CANVAS_ORIGIN
    };

    ImVec2 bottomRightOffset = {
        topLeftOffset.x + (part.regionW * part.scaleX),
        topLeftOffset.y + (part.regionH * part.scaleY)
    };

    std::array<ImVec2, 4> transformedQuad({
        topLeftOffset,
        { bottomRightOffset.x, topLeftOffset.y },
        bottomRightOffset,
        { topLeftOffset.x, bottomRightOffset.y },
    });

    ImVec2 center = {
        (topLeftOffset.x + bottomRightOffset.x) / 2.0f,
        (topLeftOffset.y + bottomRightOffset.y) / 2.0f
    };

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
        if (fmod(part.angle, 360)) {
            transformedQuad[0] = rotateVec2(transformedQuad[0], part.angle, center);
            transformedQuad[1] = rotateVec2(transformedQuad[1], part.angle, center);
            transformedQuad[2] = rotateVec2(transformedQuad[2], part.angle, center);
            transformedQuad[3] = rotateVec2(transformedQuad[3], part.angle, center);
        }

        // Key & animatable scale
        {
            transformedQuad[0].x = (transformedQuad[0].x * key->scaleX * this->scaleX) + keyCenter.x;
            transformedQuad[0].y = (transformedQuad[0].y * key->scaleY * this->scaleY) + keyCenter.y;

            transformedQuad[1].x = (transformedQuad[1].x * key->scaleX * this->scaleX) + keyCenter.x;
            transformedQuad[1].y = (transformedQuad[1].y * key->scaleY * this->scaleY) + keyCenter.y;

            transformedQuad[2].x = (transformedQuad[2].x * key->scaleX * this->scaleX) + keyCenter.x;
            transformedQuad[2].y = (transformedQuad[2].y * key->scaleY * this->scaleY) + keyCenter.y;

            transformedQuad[3].x = (transformedQuad[3].x * key->scaleX * this->scaleX) + keyCenter.x;
            transformedQuad[3].y = (transformedQuad[3].y * key->scaleY * this->scaleY) + keyCenter.y;
        }

        // Key rotation
        if (fmod(key->angle, 360)) {
            transformedQuad[0] = rotateVec2(transformedQuad[0], key->angle, keyCenter);
            transformedQuad[1] = rotateVec2(transformedQuad[1], key->angle, keyCenter);
            transformedQuad[2] = rotateVec2(transformedQuad[2], key->angle, keyCenter);
            transformedQuad[3] = rotateVec2(transformedQuad[3], key->angle, keyCenter);
        }
    
        // Key offset addition
        for (uint8_t i = 0; i < 4; i++) {
            transformedQuad[i].x += (key->positionX) * this->scaleX;
            transformedQuad[i].y += (key->positionY) * this->scaleY;
        }
    }

    return transformedQuad;
}

bool Animatable::getDoesDraw(bool allowOpacity) const {
    if (!this->cellanim || !this->currentAnimation || !this->visible)
        return false;

    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(this->currentKey->arrangementIndex);

    for (auto part : arrangement->parts) {
        if (allowOpacity) {
            if (part.opacity > 0) {
                return true;
                break;
            }
        } else
            return true;
    }

    return false;
}

void Animatable::setAnimationKeyFromPtr(RvlCellAnim::AnimationKey* key) {
    assert(this->cellanim);
    assert(key);

    this->animating = false;
    this->currentKey = key;
}

void Animatable::DrawKey(
    RvlCellAnim::AnimationKey* key,
    ImDrawList* drawList,

    int32_t partIndex,
    uint32_t colorMod,
    bool allowOpacity
) {
    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(key->arrangementIndex);

    for (uint16_t i = 0; i < arrangement->parts.size(); i++) {
        if (partIndex != -1 && partIndex != i)
            continue;

        const RvlCellAnim::ArrangementPart& part = arrangement->parts.at(i);

        if ((part.opacity == 0) && allowOpacity)
            continue; // Part not visible; don't bother drawing it.

        float mismatchScaleX = static_cast<float>(this->texture->width) / this->cellanim->textureW;
        float mismatchScaleY = static_cast<float>(this->texture->height) / this->cellanim->textureH;

        uint16_t sourceRect[4] = {
            static_cast<uint16_t>(part.regionX * mismatchScaleX),
            static_cast<uint16_t>(part.regionY * mismatchScaleY),
            static_cast<uint16_t>(part.regionW * mismatchScaleX),
            static_cast<uint16_t>(part.regionH * mismatchScaleY)
        };

        auto transformedQuad = this->getPartWorldQuad(key, i);

        ImVec2 uvTopLeft = {
            sourceRect[0] / (float)this->texture->width,
            sourceRect[1] / (float)this->texture->height
        };
        ImVec2 uvBottomRight = {
            uvTopLeft.x + (sourceRect[2] / (float)this->texture->width),
            uvTopLeft.y + (sourceRect[3] / (float)this->texture->height)
        };

        ImVec2 uvs[4] =  {
            uvTopLeft, 
            { uvBottomRight.x, uvTopLeft.y }, 
            uvBottomRight, 
            { uvTopLeft.x, uvBottomRight.y }
        };

        uint8_t modAlpha = (colorMod >> 24) & 0xFF;
        uint8_t baseAlpha =
            allowOpacity ? (
                (part.opacity * key->opacity) / 255
            ) : 255;

        drawList->AddImageQuad(
            (void*)(intptr_t)this->texture->texture,
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            uvs[0], uvs[1], uvs[2], uvs[3],
            IM_COL32(
                (colorMod >>  0) & 0xFF,
                (colorMod >>  8) & 0xFF,
                (colorMod >> 16) & 0xFF,
                
                static_cast<uint8_t>((static_cast<uint16_t>(baseAlpha) * modAlpha) / 255)
            )
        );

        /* // Draw bounding box
        drawList->AddQuad(
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            IM_COL32(255, 255, 255, 255)
        );
        */
    }
}

void Animatable::Draw(ImDrawList* drawList, bool allowOpacity) {
    assert(this->cellanim);
    assert(this->currentAnimation);

    if (!this->visible)
        return;

    this->DrawKey(this->currentKey, drawList);
}

void Animatable::DrawOnionSkin(ImDrawList* drawList, uint16_t backCount, uint16_t frontCount, uint8_t opacity) {
    assert(this->cellanim);
    assert(this->currentAnimation);

    if (!this->visible)
        return;

    uint16_t i;

    if (backCount) {
        i = this->currentKeyIndex;
        while (true) {
            i--;

            if (
                (i < 0) ||
                (i >= this->currentAnimation->keys.size()) ||
                (i < (this->currentKeyIndex - backCount))
            )
                break;

            this->DrawKey(&this->currentAnimation->keys.at(i), drawList, -1, IM_COL32(255, 64, 64, opacity));
        }
    }

    if (frontCount) {
        i = this->currentKeyIndex;
        while (true) {
            i++;

            if (
                (i >= this->currentAnimation->keys.size()) ||
                (i > (this->currentKeyIndex + frontCount))
            )
                break;

            this->DrawKey(&this->currentAnimation->keys.at(i), drawList, -1, IM_COL32(64, 255, 64, opacity));
        }
    }
}

/*
void Animatable::DrawOrigins(ImDrawList* drawList, float radius, uint32_t color) {
    assert(this->cellanim);
    assert(this->currentAnimation);

    if (!this->visible)
        return;

    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(this->currentKey->arrangementIndex);

    for (auto part : arrangement->parts) {
        float totalScaleX = this->currentKey->scaleX * this->scaleX;
        float totalScaleY = this->currentKey->scaleY * this->scaleY;

        ImVec2 topLeftOffset = {
            CANVAS_ORIGIN + ((part.positionX - CANVAS_ORIGIN) * totalScaleX) + (this->offset.x - 512),
            CANVAS_ORIGIN + ((part.positionY - CANVAS_ORIGIN) * totalScaleY) + (this->offset.y - 512),
        };

        ImVec2 bottomRightOffset = {
            topLeftOffset.x + ((part.regionW * part.scaleX) * totalScaleX),
            topLeftOffset.y + ((part.regionH * part.scaleY) * totalScaleY)
        };

        ImVec2 center = {
            (topLeftOffset.x + bottomRightOffset.x) / 2.0f,
            (topLeftOffset.y + bottomRightOffset.y) / 2.0f
        };

        drawList->AddCircleFilled(center, radius, color);
    }
}
*/