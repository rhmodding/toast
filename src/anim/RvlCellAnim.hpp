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
        uint16_t regionX{ 0 }, regionY{ 0 };
        uint16_t regionW{ 0 }, regionH{ 0 };

        Unknown32 unknown;

        int16_t positionX{ 512 }, positionY{ 512 };

        float scaleX{ 1.f }, scaleY{ 1.f };

        float angle{ 0.f };

        bool flipX{ false }, flipY{ false };

        uint8_t opacity{ 0xFFu };
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