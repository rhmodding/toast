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
    };

    struct ArrangementPart {
        uint16_t regionX, regionY;
        uint16_t regionW, regionH;

        Unknown32 unknown;

        int16_t positionX, positionY;

        float scaleX, scaleY;

        float angle;

        bool flipX, flipY;

        uint8_t opacity;
    };

    struct Arrangement {
        std::vector<ArrangementPart> parts;
    };

    struct AnimationKey {
        uint16_t arrangementIndex;

        uint16_t holdFrames;

        float scaleX, scaleY;

        float angle;

        uint8_t opacity;

        Unknown32 unknown;
    };

    struct Animation {
        std::vector<AnimationKey> keys;
    };
    
    class RvlCellAnimObject {
    public:
        uint16_t sheetIndex;

        uint16_t textureW, textureH;

        std::vector<Arrangement> arrangements;
        std::vector<Animation> animations;

        std::vector<char> Reserialize();

        RvlCellAnimObject(const char* RvlCellAnimData, const size_t dataSize);
    };

    RvlCellAnimObject* ObjectFromFile(const char *filename);
}

#endif // RVLCELLANIM_HPP