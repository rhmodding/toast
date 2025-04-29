#ifndef CellAnim_HPP
#define CellAnim_HPP

#include <vector>

#include <cstdint>
#include <cmath>

#include <algorithm>

#include "../common.hpp"

namespace CellAnim {

enum CellAnimType {
    CELLANIM_TYPE_INVALID,
    CELLANIM_TYPE_RVL,
    CELLANIM_TYPE_CTR
};

struct TransformValues {
    // On all axes.
    static constexpr int MIN_POSITION = -32768;
    static constexpr int MAX_POSITION = 32767;

    // In pixels.
    int positionX { 0 }, positionY { 0 };
    // Keys scale from the center, while arrangement parts scale from
    // their origin (top-left).
    float scaleX { 1.f }, scaleY { 1.f };
    // In degrees.
    float angle { 0.f };

    TransformValues average(const TransformValues& other) const {
        TransformValues transform {
            .positionX = std::clamp<int>(
                AVERAGE_INTS(this->positionX, other.positionX),
                MIN_POSITION, MAX_POSITION
            ),
            .positionY = std::clamp<int>(
                AVERAGE_INTS(this->positionY, other.positionY),
                MIN_POSITION, MAX_POSITION
            ),

            .scaleX = AVERAGE_FLOATS(this->scaleX, other.scaleX),
            .scaleY = AVERAGE_FLOATS(this->scaleY, other.scaleY),

            .angle = AVERAGE_FLOATS(this->angle, other.angle)
        };

        return transform;
    }

    TransformValues lerp(const TransformValues& other, float t) const {
        TransformValues transform {
            .positionX = std::clamp<int>(
                LERP_INTS(this->positionX, other.positionX, t),
                MIN_POSITION, MAX_POSITION
            ),
            .positionY = std::clamp<int>(
                LERP_INTS(this->positionY, other.positionY, t),
                MIN_POSITION, MAX_POSITION
            ),

            .scaleX = std::lerp(this->scaleX, other.scaleX, t),
            .scaleY = std::lerp(this->scaleY, other.scaleY, t),

            .angle = std::lerp(this->angle, other.angle, t)
        };

        return transform;
    }

    bool operator==(const TransformValues& other) const {
        return
            this->positionX == other.positionX &&
            this->positionY == other.positionY &&

            this->scaleX == other.scaleX &&
            this->scaleY == other.scaleY &&

            this->angle == other.angle;
    }

    bool operator!=(const TransformValues& other) const {
        return !(*this == other);
    }
};

struct CTRColor {
    float r, g, b;

    CTRColor lerp(const CTRColor& other, float t) const {
        CTRColor color {
            .r = std::lerp(this->r, other.r, t),
            .g = std::lerp(this->g, other.g, t),
            .b = std::lerp(this->b, other.b, t)
        };

        // Make sure (channel * 255.f) is an exact integer in the range 0..255.
        color.r = std::clamp(static_cast<float>(std::lroundf(color.r * 255.f)) * (1.f / 255.f), 0.f, 1.f);
        color.g = std::clamp(static_cast<float>(std::lroundf(color.g * 255.f)) * (1.f / 255.f), 0.f, 1.f);
        color.b = std::clamp(static_cast<float>(std::lroundf(color.b * 255.f)) * (1.f / 255.f), 0.f, 1.f);

        return color;
    }

    bool operator==(const CTRColor& other) const {
        return
            this->r == other.r &&
            this->g == other.g &&
            this->b == other.b;
    }
};

struct CTRQuadDepth {
    float topLeft { 0.f }, bottomLeft { 0.f };
    float topRight { 0.f }, bottomRight { 0.f };

    CTRQuadDepth lerp(const CTRQuadDepth& other, float t) const {
        CTRQuadDepth depth {
            .topLeft = std::lerp(this->topLeft, other.topLeft, t),
            .bottomLeft = std::lerp(this->bottomLeft, other.bottomLeft, t),
            .topRight = std::lerp(this->topRight, other.topRight, t),
            .bottomRight = std::lerp(this->bottomRight, other.bottomRight, t)
        };

        return depth;
    }

    bool operator==(const CTRQuadDepth& other) const {
        return
            this->topLeft == other.topLeft &&
            this->bottomLeft == other.bottomLeft &&
            this->topRight == other.topRight &&
            this->bottomRight == other.bottomRight;
    }
};

struct ArrangementPart {
    // On all axes.
    static constexpr unsigned MAX_REGION = 65535;

    static constexpr unsigned MAX_TEX_VARY = 65535;
    static constexpr unsigned MAX_ID = 255;

    unsigned regionX { 8 }, regionY { 8 };
    unsigned regionW { 32 }, regionH { 32 };

    // On RVL only.
    unsigned textureVarying { 0 };

    TransformValues transform;

    bool flipX { false }, flipY { false };

    uint8_t opacity { 0xFFu };

    // On CTR only.
    CTRColor foreColor { 1.f, 1.f, 1.f };
    // On CTR only.
    CTRColor backColor { 0.f, 0.f, 0.f };

    // On CTR only.
    CTRQuadDepth quadDepth;

    // On CTR only.
    unsigned id { 0 };

    // On CTR only.
    std::string emitterName;

    bool editorVisible { true };
    bool editorLocked { false };

    std::string editorName;

    bool operator==(const ArrangementPart& other) const {
        return
            this->regionX == other.regionX &&
            this->regionY == other.regionY &&
            this->regionW == other.regionW &&
            this->regionH == other.regionH &&

            this->textureVarying == other.textureVarying &&

            this->transform == other.transform &&

            this->flipX == other.flipX &&
            this->flipY == other.flipY &&

            this->opacity == other.opacity &&

            this->foreColor == other.foreColor &&
            this->backColor == other.backColor &&

            this->quadDepth == other.quadDepth &&

            this->id == other.id &&

            this->emitterName == other.emitterName;
    }

    bool operator!=(const ArrangementPart& other) const {
        return !(*this == other);
    }
};

struct Arrangement {
    std::vector<ArrangementPart> parts;

    int tempOffset[2] { 0, 0 };
    float tempScale[2] { 1.f, 1.f };

    bool operator==(const Arrangement& other) const {
        return other.parts == this->parts;
    }
};

struct AnimationKey {
    static constexpr unsigned MAX_ARRANGEMENT_IDX = 65535;
    static constexpr unsigned MAX_HOLD_FRAMES = 65535;

    unsigned arrangementIndex { 0 };

    // If holdFrames is zero then the key is skipped.
    unsigned holdFrames { 1 };

    TransformValues transform;

    // On CTR only.
    float translateZ { 0.f };

    uint8_t opacity { 0xFFu };

    // On CTR only.
    CTRColor foreColor { 1.f, 1.f, 1.f };
    // On CTR only.
    CTRColor backColor { 0.f, 0.f, 0.f };

    bool operator==(const AnimationKey& other) const {
        return
            this->arrangementIndex == other.arrangementIndex &&

            this->holdFrames == other.holdFrames &&

            this->transform == other.transform &&

            this->translateZ == other.translateZ &&

            this->opacity == other.opacity &&

            this->foreColor == other.foreColor &&
            this->backColor == other.backColor;
    }

    bool operator!=(const AnimationKey& other) const {
        return !(*this == other);
    }
};

struct Animation {
    std::vector<AnimationKey> keys;
    std::string name;

    // On CTR only.
    // Note: arrangement parts are matched by id.
    bool isInterpolated { false };
};

class CellAnimObject {
public:
    CellAnimObject(const unsigned char* data, const size_t dataSize);
    CellAnimObject() = default;

    bool isInitialized() const { return this->initialized; }

    CellAnimType getType() const { return this->type; }
    void setType(CellAnimType type) { this->type = type; }

    [[nodiscard]] std::vector<unsigned char> Serialize();

public:
    int sheetIndex { -1 };
    unsigned sheetW { 0 }, sheetH { 0 };

    bool usePalette { false };

    std::vector<Arrangement> arrangements;
    std::vector<Animation> animations;

private:
    bool initialized { false };

    CellAnimType type { CELLANIM_TYPE_INVALID };
};

} // namespace CellAnim

#endif // CellAnim_HPP
