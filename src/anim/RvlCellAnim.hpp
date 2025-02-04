#ifndef RVLCELLANIM_HPP
#define RVLCELLANIM_HPP

#include <vector>

#include <cstdint>
#include <cmath>

#include <memory>

#include "../common.hpp"

namespace RvlCellAnim {

struct TransformValues {
    // On all axes.
    static constexpr int MIN_POSITION = -32768;
    static constexpr int MAX_POSITION = 32767;

    // In pixels.
    int positionX { 0 }, positionY { 0 };
    // Keys: scales from center
    // Arrangement parts: scales from top-left
    float scaleX { 1.f }, scaleY { 1.f };
    // In degrees.
    float angle { 0.f };

    TransformValues average(const TransformValues& other) const {
        TransformValues transform {
            .positionX = AVERAGE_INTS(this->positionX, other.positionX),
            .positionY = AVERAGE_INTS(this->positionY, other.positionY),

            .scaleX = AVERAGE_FLOATS(this->scaleX, other.scaleX),
            .scaleY = AVERAGE_FLOATS(this->scaleY, other.scaleY),

            .angle = AVERAGE_FLOATS(this->angle, other.angle)
        };

        return transform;
    }

    TransformValues lerp(const TransformValues& other, float t) const {
        TransformValues transform {
            .positionX = int(std::lerp(this->positionX, other.positionX, t) + .5f),
            .positionY = int(std::lerp(this->positionY, other.positionY, t) + .5f),

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
    uint8_t r, g, b;
};

struct CTRDepth {
    float topLeft { 0.f }, bottomLeft { 0.f };
    float topRight { 0.f }, bottomRight { 0.f };
};

struct ArrangementPart {
    // On all axes.
    static constexpr unsigned MAX_REGION = 65535;

    static constexpr unsigned MAX_TEX_VARY = 65535;
    static constexpr unsigned MAX_ID = 255;

    unsigned regionX { 8 }, regionY { 8 };
    unsigned regionW { 32 }, regionH { 32 };

    // TODO: implement
    unsigned textureVarying { 0 };

    TransformValues transform;

    bool flipX { false }, flipY { false };

    uint8_t opacity { 0xFFu };

    // On CTR only.
    CTRColor foreColor { 0xFF, 0xFF, 0xFF };
    // On CTR only.
    CTRColor backColor { 0x00, 0x00, 0x00 };

    // On CTR only.
    CTRDepth depth3D;

    // On CTR only.
    unsigned id { 0 };

    bool editorVisible { true };
    bool editorLocked { false };

    char editorName[32] { '\0' };

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

            this->opacity == other.opacity;
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

    static constexpr unsigned MIN_HOLD_FRAMES = 1;
    static constexpr unsigned MAX_HOLD_FRAMES = 65535;

    unsigned arrangementIndex { 0 };

    unsigned holdFrames { 1 };

    TransformValues transform;

    uint8_t opacity { 0xFFu };

    // On CTR only.
    CTRColor foreColor { 0xFF, 0xFF, 0xFF };
    // On CTR only.
    CTRColor backColor { 0x00, 0x00, 0x00 };

    // On CTR only.
    CTRDepth depth3D;

    bool operator==(const AnimationKey& other) const {
        return
            this->arrangementIndex == other.arrangementIndex &&

            this->holdFrames == other.holdFrames &&

            this->transform == other.transform &&

            this->opacity == other.opacity;
    }

    bool operator!=(const AnimationKey& other) const {
        return !(*this == other);
    }
};

struct Animation {
    std::vector<AnimationKey> keys;
    std::string name;
};

class RvlCellAnimObject {
public:
    RvlCellAnimObject(const unsigned char* data, const size_t dataSize);
    RvlCellAnimObject() = default;

    bool isInitialized() const { return this->initialized; };

    std::vector<unsigned char> Reserialize();

public:
    unsigned sheetIndex;
    unsigned sheetW, sheetH;
    
    bool usePalette;

    std::vector<Arrangement> arrangements;
    std::vector<Animation> animations;

private:
    bool initialized { false };
};

std::shared_ptr<RvlCellAnimObject> readRvlCellAnimFile(const char *filePath);

} // namespace RvlCellAnim

#endif // RVLCELLANIM_HPP
