#ifndef RVLCELLANIM_HPP
#define RVLCELLANIM_HPP

#include <vector>

#include <cstdint>
#include <cmath>

#include <memory>

#include "../common.hpp"

namespace RvlCellAnim {

struct TransformValues {
    int16_t positionX { 0 }, positionY { 0 };
    float scaleX { 1.f }, scaleY { 1.f };
    float angle { 0.f }; // Angle in degrees

    TransformValues average(const TransformValues& other) const {
        TransformValues transform {
            .positionX = (int16_t)AVERAGE_INTS(this->positionX, other.positionX),
            .positionY = (int16_t)AVERAGE_INTS(this->positionY, other.positionY),

            .scaleX = AVERAGE_FLOATS(this->scaleX, other.scaleX),
            .scaleY = AVERAGE_FLOATS(this->scaleY, other.scaleY),

            .angle = AVERAGE_FLOATS(this->angle, other.angle)
        };

        return transform;
    }

    TransformValues lerp(const TransformValues& other, float t) const {
        TransformValues transform {
            .positionX = (int16_t)std::lerp(this->positionX, other.positionX, t),
            .positionY = (int16_t)std::lerp(this->positionY, other.positionY, t),

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
} __attribute__((packed));

struct ArrangementPart {
    uint16_t regionX { 8 }, regionY { 8 };
    uint16_t regionW { 32 }, regionH { 32 };

    uint16_t textureVarying { 0 };

    TransformValues transform;

    bool flipX { false }, flipY { false };

    uint8_t opacity { 0xFFu };

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
    uint16_t arrangementIndex { 0 };

    uint16_t holdFrames { 1 };

    TransformValues transform;

    uint8_t opacity { 0xFFu };

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
};

class RvlCellAnimObject {
public:
    bool ok { false };

    uint16_t sheetIndex;

    uint16_t textureW, textureH;
    
    bool usePalette;

    std::vector<Arrangement> arrangements;
    std::vector<Animation> animations;

    std::vector<unsigned char> Reserialize();

    RvlCellAnimObject(const unsigned char* RvlCellAnimData, const size_t dataSize);
};

std::shared_ptr<RvlCellAnimObject> readRvlCellAnimFile(const char *filePath);

} // namespace RvlCellAnim

#endif // RVLCELLANIM_HPP
