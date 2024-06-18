#ifndef RVLCELLANIM_HPP
#define RVLCELLANIM_HPP

#include <vector>
#include <cstdint>

namespace RvlCellAnim {
    union Unknown32 {
        uint32_t u32;
        uint16_t u16[2];
        uint8_t u8[4];

        Unknown32() : u32(0) {}

        bool operator==(const Unknown32& other) const {
            return this->u32 == other.u32;
        }
    };

    struct ArrangementPart {
        uint16_t regionX{ 0 }, regionY{ 0 };
        uint16_t regionW{ 0 }, regionH{ 0 };

        Unknown32 unknown;

        int16_t positionX{ 512 }, positionY{ 512 };

        float scaleX{ 1.f }, scaleY{ 1.f };

        float angle{ 0.f };

        bool flipX{ false }, flipY{ false };

        uint8_t opacity{ 0xFFu };

        bool operator==(const ArrangementPart& other) const {
            return
                this->regionX == other.regionX &&
                this->regionY == other.regionY &&
                this->regionW == other.regionW &&
                this->regionH == other.regionH &&

                this->unknown == other.unknown &&

                this->positionX == other.positionX &&
                this->positionY == other.positionY &&

                this->scaleX == other.scaleX &&
                this->scaleY == other.scaleY &&

                this->angle == other.angle &&

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
    };

    struct AnimationKey {
        uint16_t arrangementIndex{ 0 };

        uint16_t holdFrames{ 1 };

        float scaleX{ 1.f }, scaleY{ 1.f };

        float angle{ 0.f };

        uint8_t opacity{ 0xFFu };

        uint8_t offsetLeft{ 0 }, offsetRight{ 0 };
        uint8_t offsetTop{ 0 }, offsetBottom{ 0 };

        bool operator==(const AnimationKey& other) const {
            return
                this->arrangementIndex == other.arrangementIndex &&

                this->holdFrames == other.holdFrames &&

                this->scaleX == other.scaleX &&
                this->scaleY == other.scaleY &&

                this->angle == other.angle &&

                this->opacity == other.opacity &&

                this->offsetLeft == other.offsetLeft &&
                this->offsetRight == other.offsetRight &&
                this->offsetTop == other.offsetTop &&
                this->offsetBottom == other.offsetBottom;
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
        bool ok{ false };

        uint16_t sheetIndex;

        uint16_t textureW, textureH;

        Unknown32 unknown;

        std::vector<Arrangement> arrangements;
        std::vector<Animation> animations;

        std::vector<unsigned char> Reserialize();

        RvlCellAnimObject(const unsigned char* RvlCellAnimData, const size_t dataSize);
    };

    RvlCellAnimObject* ObjectFromFile(const char *filename);
}

#endif // RVLCELLANIM_HPP