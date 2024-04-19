#include "Animatable.hpp"

#include <assert.h>

#include "../AppState.hpp"

#include "../common.hpp"

#define CANVAS_ORIGIN 512

#define M_PI 3.14159265358979323846

void Animatable::setAnimation(uint16_t animIndex) {
    assert(this->cellanim);

    assert(this->cellanim->animations.size() >= animIndex);

    this->currentAnimation = &this->cellanim->animations.at(animIndex);
    this->currentAnimationIndex = animIndex;

    this->currentKey = &this->currentAnimation->keys.at(0);
    this->currentKeyIndex = 0;

    this->holdKey = this->currentKey->holdFrames;
}

void Animatable::setAnimationKey(uint16_t keyIndex) {
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
bool Animatable::isAnimating() const {
    return this->animating;
}

void Animatable::Update() {
    if (!this->animating)
        return;

    if (this->holdKey < 1) {
        // shut up compiler !!
        if ((uint16_t)(this->currentKeyIndex + 1) >= (uint16_t)this->currentAnimation->keys.size()) {
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

int16_t Animatable::getHoldFramesLeft() const {
    return this->holdKey;
}

ImVec2 Animatable::getPartWorldSpace(uint16_t animationIndex, uint16_t keyIndex, uint16_t partIndex) const {
    assert(this->cellanim);

    RvlCellAnim::AnimationKey* key = &this->cellanim->animations.at(animationIndex).keys.at(keyIndex);
    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(key->arrangementIndex);
    RvlCellAnim::ArrangementPart part = arrangement->parts.at(partIndex);

    {
        float totalScaleX = key->scaleX * this->scaleX;
        float totalScaleY = key->scaleY * this->scaleY;

        ImVec2 point = {
            CANVAS_ORIGIN + ((part.positionX - CANVAS_ORIGIN) * totalScaleX) + (this->offset.x - 512),
            CANVAS_ORIGIN + ((part.positionY - CANVAS_ORIGIN) * totalScaleY) + (this->offset.y - 512),
        };

        return point;
    }
}

ImVec2 rotateVec2(ImVec2 v, float angle, ImVec2 origin) {
    float s = static_cast<float>(std::sin(angle * (M_PI / 180)));
    float c = static_cast<float>(std::cos(angle * (M_PI / 180)));

    v.x -= origin.x;
    v.y -= origin.y;

    float x = v.x * c - v.y * s;
    float y = v.x * s + v.y * c;

    return { x + origin.x, y + origin.y };
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

void Animatable::Draw(ImDrawList* drawList, bool allowOpacity) {
    assert(this->cellanim);
    assert(this->currentAnimation);

    if (!this->visible)
        return;

    RvlCellAnim::Arrangement* arrangement = &this->cellanim->arrangements.at(this->currentKey->arrangementIndex);

    for (auto part : arrangement->parts) {
        if ((part.opacity == 0) && allowOpacity)
            continue; // Part not visible; don't bother drawing it.

        uint16_t sourceRect[4] = { part.regionX, part.regionY, part.regionW, part.regionH };

        // float totalScaleX = this->currentKey->scaleX * this->scaleX;
        // float totalScaleY = this->currentKey->scaleY * this->scaleY;

        ImVec2 keyCenter = {
            CANVAS_ORIGIN + (this->offset.x - 512),
            CANVAS_ORIGIN + (this->offset.y - 512)
        };

        ImVec2 topLeftOffset = {
            CANVAS_ORIGIN + (part.positionX - CANVAS_ORIGIN) + (this->offset.x - 512),
            CANVAS_ORIGIN + (part.positionY - CANVAS_ORIGIN) + (this->offset.y - 512),
        };

        ImVec2 bottomRightOffset = {
            topLeftOffset.x + (part.regionW * part.scaleX),
            topLeftOffset.y + (part.regionH * part.scaleY)
        };

        ImVec2 transformedQuad[4] = {
            topLeftOffset,
            { bottomRightOffset.x, topLeftOffset.y },
            bottomRightOffset,
            { topLeftOffset.x, bottomRightOffset.y },
        };

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
                std::swap(transformedQuad[0], transformedQuad[2]);
                std::swap(transformedQuad[1], transformedQuad[3]);
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
                transformedQuad[0].x = ((transformedQuad[0].x - keyCenter.x) * this->currentKey->scaleX * this->scaleX) + keyCenter.x;
                transformedQuad[0].y = ((transformedQuad[0].y - keyCenter.y) * this->currentKey->scaleY * this->scaleY) + keyCenter.y;

                transformedQuad[1].x = ((transformedQuad[1].x - keyCenter.x) * this->currentKey->scaleX * this->scaleX) + keyCenter.x;
                transformedQuad[1].y = ((transformedQuad[1].y - keyCenter.y) * this->currentKey->scaleY * this->scaleY) + keyCenter.y;

                transformedQuad[2].x = ((transformedQuad[2].x - keyCenter.x) * this->currentKey->scaleX * this->scaleX) + keyCenter.x;
                transformedQuad[2].y = ((transformedQuad[2].y - keyCenter.y) * this->currentKey->scaleY * this->scaleY) + keyCenter.y;

                transformedQuad[3].x = ((transformedQuad[3].x - keyCenter.x) * this->currentKey->scaleX * this->scaleX) + keyCenter.x;
                transformedQuad[3].y = ((transformedQuad[3].y - keyCenter.y) * this->currentKey->scaleY * this->scaleY) + keyCenter.y;
            }

            // Key rotation
            if (fmod(this->currentKey->angle, 360)) {
                transformedQuad[0] = rotateVec2(transformedQuad[0], this->currentKey->angle, keyCenter);
                transformedQuad[1] = rotateVec2(transformedQuad[1], this->currentKey->angle, keyCenter);
                transformedQuad[2] = rotateVec2(transformedQuad[2], this->currentKey->angle, keyCenter);
                transformedQuad[3] = rotateVec2(transformedQuad[3], this->currentKey->angle, keyCenter);
            }
        }

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

        drawList->AddImageQuad(
            (void*)(intptr_t)this->texture->texture,
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            uvs[0], uvs[1], uvs[2], uvs[3],
            IM_COL32(255, 255, 255, allowOpacity ? ( (part.opacity * this->currentKey->opacity) / 255 ) : 255)
        );

        /* // Draw bounding box
        drawList->AddQuad(
            transformedQuad[0], transformedQuad[1], transformedQuad[2], transformedQuad[3],
            IM_COL32(255, 255, 255, 255)
        );
        */
    }
}

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