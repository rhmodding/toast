#ifndef CELL_ANIM_HPP
#define CELL_ANIM_HPP

#include <vector>

#include <cstdint>
#include <cmath>

#include <algorithm>

#include "../Macro.hpp"

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

    TransformValues average(const TransformValues& rhs) const {
        TransformValues transform {
            .positionX = std::clamp<int>(
                AVERAGE_INTS(positionX, rhs.positionX),
                MIN_POSITION, MAX_POSITION
            ),
            .positionY = std::clamp<int>(
                AVERAGE_INTS(positionY, rhs.positionY),
                MIN_POSITION, MAX_POSITION
            ),

            .scaleX = AVERAGE_FLOATS(scaleX, rhs.scaleX),
            .scaleY = AVERAGE_FLOATS(scaleY, rhs.scaleY),

            .angle = AVERAGE_FLOATS(angle, rhs.angle)
        };

        return transform;
    }

    TransformValues lerp(const TransformValues& rhs, float t) const {
        TransformValues transform {
            .positionX = std::clamp<int>(
                LERP_INTS(positionX, rhs.positionX, t),
                MIN_POSITION, MAX_POSITION
            ),
            .positionY = std::clamp<int>(
                LERP_INTS(positionY, rhs.positionY, t),
                MIN_POSITION, MAX_POSITION
            ),

            .scaleX = std::lerp(scaleX, rhs.scaleX, t),
            .scaleY = std::lerp(scaleY, rhs.scaleY, t),

            .angle = std::lerp(angle, rhs.angle, t)
        };

        return transform;
    }

    bool operator==(const TransformValues& rhs) const {
        return
            positionX == rhs.positionX &&
            positionY == rhs.positionY &&

            scaleX == rhs.scaleX &&
            scaleY == rhs.scaleY &&

            angle == rhs.angle;
    }

    bool operator!=(const TransformValues& rhs) const {
        return !(*this == rhs);
    }
};

struct CTRColor {
    float r, g, b;

    CTRColor lerp(const CTRColor& rhs, float t) const {
        CTRColor color {
            .r = std::lerp(r, rhs.r, t),
            .g = std::lerp(g, rhs.g, t),
            .b = std::lerp(b, rhs.b, t)
        };

        // Make sure (channel * 255.f) is an exact integer in the range 0..255.
        color.r = std::clamp(static_cast<float>(std::lroundf(color.r * 255.f)) * (1.f / 255.f), 0.f, 1.f);
        color.g = std::clamp(static_cast<float>(std::lroundf(color.g * 255.f)) * (1.f / 255.f), 0.f, 1.f);
        color.b = std::clamp(static_cast<float>(std::lroundf(color.b * 255.f)) * (1.f / 255.f), 0.f, 1.f);

        return color;
    }

    bool operator==(const CTRColor& rhs) const {
        return
            r == rhs.r &&
            g == rhs.g &&
            b == rhs.b;
    }
};

struct CTRQuadDepth {
    float topLeft { 0.f }, bottomLeft { 0.f };
    float topRight { 0.f }, bottomRight { 0.f };

    CTRQuadDepth lerp(const CTRQuadDepth& rhs, float t) const {
        CTRQuadDepth depth {
            .topLeft = std::lerp(topLeft, rhs.topLeft, t),
            .bottomLeft = std::lerp(bottomLeft, rhs.bottomLeft, t),
            .topRight = std::lerp(topRight, rhs.topRight, t),
            .bottomRight = std::lerp(bottomRight, rhs.bottomRight, t)
        };

        return depth;
    }

    bool operator==(const CTRQuadDepth& rhs) const {
        return
            topLeft == rhs.topLeft &&
            bottomLeft == rhs.bottomLeft &&
            topRight == rhs.topRight &&
            bottomRight == rhs.bottomRight;
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

    bool operator==(const ArrangementPart& rhs) const {
        return
            regionX == rhs.regionX &&
            regionY == rhs.regionY &&
            regionW == rhs.regionW &&
            regionH == rhs.regionH &&

            textureVarying == rhs.textureVarying &&

            transform == rhs.transform &&

            flipX == rhs.flipX &&
            flipY == rhs.flipY &&

            opacity == rhs.opacity &&

            foreColor == rhs.foreColor &&
            backColor == rhs.backColor &&

            quadDepth == rhs.quadDepth &&

            id == rhs.id &&

            emitterName == rhs.emitterName;
    }

    bool operator!=(const ArrangementPart& rhs) const {
        return !(*this == rhs);
    }
};

struct Arrangement {
    std::vector<ArrangementPart> parts;

    // Temporary & editor-specific: used for arrangement scale preview.
    int tempOffset[2] { 0, 0 };
    float tempScale[2] { 1.f, 1.f };

    bool operator==(const Arrangement& rhs) const {
        return rhs.parts == parts;
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

    bool operator==(const AnimationKey& rhs) const {
        return
            arrangementIndex == rhs.arrangementIndex &&

            holdFrames == rhs.holdFrames &&

            transform == rhs.transform &&

            translateZ == rhs.translateZ &&

            opacity == rhs.opacity &&

            foreColor == rhs.foreColor &&
            backColor == rhs.backColor;
    }

    bool operator!=(const AnimationKey& rhs) const {
        return !(*this == rhs);
    }
};

struct Animation {
    std::vector<AnimationKey> keys;
    std::string name;

    // On CTR only.
    // Note: arrangement parts are matched by ID, not index.
    bool isInterpolated { false };
};

class CellAnimObject {
public:
    CellAnimObject(const unsigned char* data, const size_t dataSize);
    CellAnimObject() = default;

    bool isInitialized() const { return mInitialized; }

    CellAnimType getType() const { return mType; }
    void setType(CellAnimType type) { mType = type; }

    int getSheetIndex() const { return mSheetIndex; }
    void setSheetIndex(int sheetIndex) {
        if (sheetIndex < 0)
            mSheetIndex = -1;
        else
            mSheetIndex = sheetIndex;
    }

    unsigned getSheetWidth() const { return mSheetW; }
    void setSheetWidth(unsigned width) { mSheetW = width; }

    unsigned getSheetHeight() const { return mSheetH; }
    void setSheetHeight(unsigned height) { mSheetH = height; }

    bool getUsePalette() const { return mUsePalette; }
    void setUsePalette(bool usePalette) { mUsePalette = usePalette; }

    std::vector<Arrangement>& getArrangements() { return mArrangements; }
    const std::vector<Arrangement>& getArrangements() const { return mArrangements; }

    Arrangement& getArrangement(unsigned arrangementIndex) {
        if (arrangementIndex >= mArrangements.size()) {
            throw std::runtime_error(
                "CellAnimObject::getArrangement: arrangement index out of bounds (" +
                std::to_string(arrangementIndex) + " >= " + std::to_string(mArrangements.size()) + ")"
            );
        }
        return mArrangements[arrangementIndex];
    }
    const Arrangement& getArrangement(unsigned arrangementIndex) const {
        if (arrangementIndex >= mArrangements.size()) {
            throw std::runtime_error(
                "CellAnimObject::getArrangement: arrangement index out of bounds (" +
                std::to_string(arrangementIndex) + " >= " + std::to_string(mArrangements.size()) + ")"
            );
        }
        return mArrangements[arrangementIndex];
    }

    std::vector<Animation>& getAnimations() { return mAnimations; }
    const std::vector<Animation>& getAnimations() const { return mAnimations; }

    Animation& getAnimation(unsigned animationIndex) {
        if (animationIndex >= mAnimations.size()) {
            throw std::runtime_error(
                "CellAnimObject::getAnimation: animation index out of bounds (" +
                std::to_string(animationIndex) + " >= " + std::to_string(mAnimations.size()) + ")"
            );
        }
        return mAnimations[animationIndex];
    }
    const Animation& getAnimation(unsigned animationIndex) const {
        if (animationIndex >= mAnimations.size()) {
            throw std::runtime_error(
                "CellAnimObject::getAnimation: animation index out of bounds (" +
                std::to_string(animationIndex) + " >= " + std::to_string(mAnimations.size()) + ")"
            );
        }
        return mAnimations[animationIndex];
    }

    [[nodiscard]] std::vector<unsigned char> Serialize();

    std::vector<Arrangement>::iterator duplicateArrangement(const Arrangement& arrangement);
    std::vector<Arrangement>::iterator duplicateArrangement(unsigned arrangementIndex) {
        return duplicateArrangement(mArrangements.at(arrangementIndex));
    }

    bool isArrangementUnique(const Arrangement& arrangement) const;
    bool isArrangementUnique(unsigned arrangementIndex) const {
        return isArrangementUnique(mArrangements.at(arrangementIndex));
    }

private:
    bool InitImpl_Rvl(const unsigned char* data, const size_t dataSize);
    bool InitImpl_Ctr(const unsigned char* data, const size_t dataSize);

    std::vector<unsigned char> SerializeImpl_Rvl();
    std::vector<unsigned char> SerializeImpl_Ctr();

private:
    bool mInitialized { false };

    CellAnimType mType { CELLANIM_TYPE_INVALID };

    int mSheetIndex { -1 };
    unsigned mSheetW { 0 }, mSheetH { 0 };

    // This flag loads the selected sheet as a paletted texture in-game.
    //     - If the sheet is non-paletted and usePalette is true, the texture
    //       will not appear.
    //     - This also applies if usePalette is false and the texture is paletted.
    bool mUsePalette { false };

    std::vector<Arrangement> mArrangements;
    std::vector<Animation> mAnimations;
};

} // namespace CellAnim

#endif // CELL_ANIM_HPP
