#ifndef REVCELLANIM_HPP
#define REVCELLANIM_HPP

#include <vector>
#include <cstdint>

namespace RevCellAnim {
    struct ArrangementPart {
        uint16_t regionX;
        uint16_t regionY;
        uint16_t regionW;
        uint16_t regionH;

        int16_t positionX;
        int16_t positionY;

        float scaleX;
        float scaleY;

        float angle;

        bool flipX;
        bool flipY;

        uint8_t opacity;
    };

    struct Arrangement {
        std::vector<ArrangementPart> parts;
    };

    struct AnimationKey {
        uint16_t arrangementIndex;

        uint16_t holdFrames;

        float scaleX;
        float scaleY;

        float angle;

        uint8_t opacity;
    };

    struct Animation {
        std::vector<AnimationKey> keys;
    };
    
    class RevCellAnimObject {
    public:
        uint16_t sheetIndex;

        std::vector<Arrangement> arrangements;
        std::vector<Animation> animations;

        RevCellAnimObject(const char* revCellAnimData, const size_t dataSize);
    };

    RevCellAnimObject* ObjectFromFile(const char *filename);
}

#endif // REVCELLANIM_HPP