#ifndef CTRCELLANIM_HPP
#define CTRCELLANIM_HPP

#include <vector>
#include <cstdint>
#include <string>

namespace CtrCellAnim {
    // All values are from 0.f - 1.f
    struct Color { float r, g, b, a; };

    struct StereoDepth {
        float topLeft;
        float bottomLeft;
        float topRight;
        float bottomRight;
    };

    struct ArrangementPart {
        uint16_t regionX, regionY;
        uint16_t regionW, regionH;

        int16_t positionX, positionY;

        float scaleX, scaleY;

        float angle;

        bool flipX, flipY;

        Color multiplyColor, screenColor;

        uint8_t opacity;

        // Used as an ID to programmatically control parts from the game.
        uint8_t designation;

        StereoDepth depth;
    };

    struct Arrangement {
        std::vector<ArrangementPart> parts;
    };

    struct AnimationKey {
        uint16_t arrangementIndex;

        uint16_t holdFrames;

        int16_t positionX, positionY;

        float depth;

        float scaleX, scaleY;

        float angle;

        Color multiplyColor;

        uint8_t opacity;
    };

    struct Animation {
        std::string animationName;

        // TODO: figure this out later
        int32_t interpolation;

        std::vector<AnimationKey> keys;
    };
    
    class CtrCellAnimObject {
    public:
        uint16_t sheetIndex;

        // 1024x1024 is the maximum by hardware limitation on the 3DS
        uint16_t textureW, textureH;

        std::vector<Arrangement> arrangements;
        std::vector<Animation> animations;

        CtrCellAnimObject(const char* CtrCellAnimData, const size_t dataSize);
    };

    CtrCellAnimObject* ObjectFromFile(const char *filename);
}

#endif // CTRCELLANIM_HPP