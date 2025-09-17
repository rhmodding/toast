#include "CellAnim.hpp"

#include <cstring>

#include <map>

#include "Logging.hpp"

#include "Macro.hpp"

// 12 Mar 2010
constexpr uint32_t RCAD_REVISION_DATE = 20100312;

// 7 Oct 2013
constexpr uint32_t CCAD_REVISION_DATE = 20131007;

struct RvlCellAnimHeader {
    // Compare to RCAD_REVISION_DATE.
    uint32_t revisionDate { BYTESWAP_32(RCAD_REVISION_DATE) };

    // Load textures in paletted (CI) mode; also enables varyings.
    uint8_t usePalette;

    uint8_t _pad24[3] { 0x00, 0x00, 0x00 };

    // Index into cellanim.tpl for the associated sheet.
    uint16_t sheetIndex;

    // Unused value (never referenced in the game's code).
    uint16_t _unused { 0x0000 };

    uint16_t sheetW; // Sheet width in relation to UV regions.
    uint16_t sheetH; // Sheet height in relation to UV regions.

    uint16_t arrangementCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));
struct CtrCellAnimHeader {
    // Compare to CCAD_REVISION_DATE.
    uint32_t revisionDate { CCAD_REVISION_DATE };

    uint16_t sheetW; // Sheet width in relation to UV regions.
    uint16_t sheetH; // Sheet height in relation to UV regions.

    uint16_t arrangementCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

template <bool isArrangementPart>
struct RvlTransformValues {
    int16_t positionX { 0 }, positionY { 0 }; // In pixels.
    float scaleX { 1.f }, scaleY { 1.f };
    float angle { 0.f }; // In degrees.

    RvlTransformValues() = default;

    RvlTransformValues(const CellAnim::TransformValues& transformValues) {
        const int additive = (isArrangementPart ? 512 : 0);

        positionX = BYTESWAP_16(static_cast<int16_t>(transformValues.position.x + additive));
        positionY = BYTESWAP_16(static_cast<int16_t>(transformValues.position.y + additive));
        scaleX = BYTESWAP_FLOAT(transformValues.scale.x);
        scaleY = BYTESWAP_FLOAT(transformValues.scale.y);
        angle = BYTESWAP_FLOAT(transformValues.angle);
    }

    CellAnim::TransformValues toTransformValues() const {
        CellAnim::TransformValues transformValues;

        const int additive = (isArrangementPart ? -512 : 0);

        transformValues.position.x = static_cast<int>(static_cast<int16_t>(BYTESWAP_16(positionX) + additive));
        transformValues.position.y = static_cast<int>(static_cast<int16_t>(BYTESWAP_16(positionY) + additive));
        transformValues.scale.x = BYTESWAP_FLOAT(scaleX);
        transformValues.scale.y = BYTESWAP_FLOAT(scaleY);
        transformValues.angle = BYTESWAP_FLOAT(angle);

        return transformValues;
    }
} __attribute__((packed));

struct RvlArrangementPart {
    uint16_t cellX; // X position of UV region in spritesheet
    uint16_t cellY; // Y position of UV region in spritesheet
    uint16_t cellW; // Width of UV region in spritesheet
    uint16_t cellH; // Height of UV region in spritesheet

    // Additive to the texture index (if usePalette is true).
    uint16_t textureVarying;

    uint16_t _pad16 { 0x0000 };

    RvlTransformValues<true> transform;

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

    uint8_t opacity;

    uint8_t _pad8 { 0x00 };

    RvlArrangementPart() = default;
    RvlArrangementPart(const CellAnim::ArrangementPart& arrangementPart) {
        cellX = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.cellOrigin.x));
        cellY = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.cellOrigin.y));
        cellW = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.cellSize.x));
        cellH = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.cellSize.y));

        textureVarying = BYTESWAP_16(arrangementPart.textureVarying);

        transform = RvlTransformValues<true>(arrangementPart.transform);

        flipX = arrangementPart.flipX ? 0x01 : 0x00;
        flipY = arrangementPart.flipY ? 0x01 : 0x00;

        opacity = arrangementPart.opacity;
    }

    CellAnim::ArrangementPart toArrangementPart() const {
        return CellAnim::ArrangementPart {
            .cellOrigin = CellAnim::UintVec2 {
                BYTESWAP_16(cellX),
                BYTESWAP_16(cellY)
            },
            .cellSize = CellAnim::UintVec2 {
                BYTESWAP_16(cellW),
                BYTESWAP_16(cellH)
            },

            .textureVarying = BYTESWAP_16(textureVarying),

            .transform = transform.toTransformValues(),

            .flipX = flipX != 0x00,
            .flipY = flipY != 0x00,

            .opacity = opacity
        };
    }
} __attribute__((packed));
struct CtrArrangementPart {
    uint16_t cellX; // X position of UV region in spritesheet
    uint16_t cellY; // Y position of UV region in spritesheet
    uint16_t cellW; // Width of UV region in spritesheet
    uint16_t cellH; // Height of UV region in spritesheet

    int16_t positionX { 0 }, positionY { 0 }; // In pixels.
    float scaleX { 1.f }, scaleY { 1.f };
    float angle { 0.f }; // In degrees.

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

    uint8_t foreColor[3]; // RGB
    uint8_t backColor[3]; // RGB

    uint8_t opacity;

    // Every element seems to equal 0xFF.
    uint8_t _unknown0[12];

    uint8_t id;

    // 0xFF means no ID.
    uint8_t emitterId { 0xFF };

    uint8_t _pad8 { 0x00 };

    float depthTL { 0.f }, depthBL { 0.f };
    float depthTR { 0.f }, depthBR { 0.f };

    CtrArrangementPart() = default;
    CtrArrangementPart(const CellAnim::ArrangementPart& arrangementPart) {
        cellX = arrangementPart.cellOrigin.x;
        cellY = arrangementPart.cellOrigin.y;
        cellW = arrangementPart.cellSize.x;
        cellH = arrangementPart.cellSize.y;

        positionX = static_cast<int16_t>(arrangementPart.transform.position.x + 512);
        positionY = static_cast<int16_t>(arrangementPart.transform.position.y + 512);
        scaleX = arrangementPart.transform.scale.x;
        scaleY = arrangementPart.transform.scale.y;
        angle = arrangementPart.transform.angle;

        flipX = arrangementPart.flipX ? 0x01 : 0x00;
        flipY = arrangementPart.flipY ? 0x01 : 0x00;

        foreColor[0] = ROUND_FLOAT(arrangementPart.foreColor.r * 255.f);
        foreColor[1] = ROUND_FLOAT(arrangementPart.foreColor.g * 255.f);
        foreColor[2] = ROUND_FLOAT(arrangementPart.foreColor.b * 255.f);
        backColor[0] = ROUND_FLOAT(arrangementPart.backColor.r * 255.f);
        backColor[1] = ROUND_FLOAT(arrangementPart.backColor.g * 255.f);
        backColor[2] = ROUND_FLOAT(arrangementPart.backColor.b * 255.f);

        opacity = arrangementPart.opacity;

        memset(_unknown0, 0xFF, 12);

        id = arrangementPart.id;

        // emitterId = arrangementPart.emitterId;

        depthTL = arrangementPart.quadDepth.topLeft;
        depthBL = arrangementPart.quadDepth.bottomLeft;
        depthTR = arrangementPart.quadDepth.topRight;
        depthBR = arrangementPart.quadDepth.bottomRight;
    }

    CellAnim::ArrangementPart toArrangementPart() const {
        return CellAnim::ArrangementPart {
            .cellOrigin = CellAnim::UintVec2 {
                cellX,
                cellY
            },
            .cellSize = CellAnim::UintVec2 {
                cellW,
                cellH
            },

            .transform = CellAnim::TransformValues {
                .position = CellAnim::IntVec2 {
                    static_cast<int>(static_cast<int16_t>(positionX - 512)),
                    static_cast<int>(static_cast<int16_t>(positionY - 512))
                },
                .scale = CellAnim::FltVec2 {
                    scaleX, scaleY
                },
                .angle = angle,
            },

            .flipX = flipX != 0x00,
            .flipY = flipY != 0x00,

            .opacity = opacity,

            .foreColor = CellAnim::CTRColor (
                foreColor[0] / 255.f,
                foreColor[1] / 255.f,
                foreColor[2] / 255.f
            ),
            .backColor = CellAnim::CTRColor (
                backColor[0] / 255.f,
                backColor[1] / 255.f,
                backColor[2] / 255.f
            ),

            .quadDepth = CellAnim::CTRQuadDepth {
                .topLeft = depthTL,
                .bottomLeft = depthBL,
                .topRight = depthTR,
                .bottomRight = depthBR,
            },

            .id = id

            // emitterName cannot be derived
        };
    }
} __attribute__((packed));

struct RvlArrangement {
    // Amount of parts in the arrangement
    uint16_t partsCount;
    uint16_t _pad16 { 0x0000 };

    RvlArrangementPart parts[0];
} __attribute__((packed));
struct CtrArrangement {
    // Amount of parts in the arrangement
    uint16_t partsCount;
    uint16_t _pad16 { 0x0000 };

    CtrArrangementPart parts[0];
} __attribute__((packed));

struct AnimationsHeader {
    // Amount of animations
    uint16_t animationCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

struct RvlAnimationKey {
    uint16_t arrangementIndex;

    // Amount of frames the key is held for (0 is allowed).
    uint16_t holdFrames;

    RvlTransformValues<false> transform;

    uint8_t opacity;

    uint8_t _pad24[3] { 0x00, 0x00, 0x00 };

    RvlAnimationKey() = default;
    RvlAnimationKey(const CellAnim::AnimationKey& animationKey) {
        arrangementIndex = BYTESWAP_16(animationKey.arrangementIndex);
        holdFrames = BYTESWAP_16(animationKey.holdFrames);

        transform = RvlTransformValues<false>(animationKey.transform);

        opacity = animationKey.opacity;
    }

    CellAnim::AnimationKey toAnimationKey() const {
        return CellAnim::AnimationKey {
            .arrangementIndex = BYTESWAP_16(arrangementIndex),

            .holdFrames = BYTESWAP_16(holdFrames),

            .transform = transform.toTransformValues(),

            .opacity = opacity
        };
    }
} __attribute__((packed));
struct CtrAnimationKey {
    uint16_t arrangementIndex;

    // Amount of frames the key is held for (0 is invalid).
    uint16_t holdFrames;

    int16_t positionX { 0 }, positionY { 0 }; // In pixels.
    float positionZ;

    float scaleX { 1.f }, scaleY { 1.f };
    float angle { 0.f }; // In degrees.

    uint8_t foreColor[3]; // RGB
    uint8_t backColor[3]; // RGB

    uint8_t opacity;

    uint8_t _pad8 { 0x00 };

    CtrAnimationKey() = default;
    CtrAnimationKey(const CellAnim::AnimationKey& animationKey) {
        arrangementIndex = animationKey.arrangementIndex;
        holdFrames = animationKey.holdFrames;

        positionX = animationKey.transform.position.x;
        positionY = animationKey.transform.position.y;

        positionZ = animationKey.translateZ;

        scaleX = animationKey.transform.scale.x;
        scaleY = animationKey.transform.scale.y;
        angle = animationKey.transform.angle;

        foreColor[0] = ROUND_FLOAT(animationKey.foreColor.r * 255.f);
        foreColor[1] = ROUND_FLOAT(animationKey.foreColor.g * 255.f);
        foreColor[2] = ROUND_FLOAT(animationKey.foreColor.b * 255.f);
        backColor[0] = ROUND_FLOAT(animationKey.backColor.r * 255.f);
        backColor[1] = ROUND_FLOAT(animationKey.backColor.g * 255.f);
        backColor[2] = ROUND_FLOAT(animationKey.backColor.b * 255.f);

        opacity = animationKey.opacity;
    }

    CellAnim::AnimationKey toAnimationKey() const {
        return CellAnim::AnimationKey {
            .arrangementIndex = arrangementIndex,

            .holdFrames = holdFrames,

            .transform = CellAnim::TransformValues {
                .position = CellAnim::IntVec2 { positionX, positionY },
                .scale = CellAnim::FltVec2 { scaleX, scaleY },
                .angle = angle,
            },

            .translateZ = positionZ,

            .opacity = opacity,

            .foreColor = CellAnim::CTRColor (
                foreColor[0] / 255.f,
                foreColor[1] / 255.f,
                foreColor[2] / 255.f
            ),
            .backColor = CellAnim::CTRColor (
                backColor[0] / 255.f,
                backColor[1] / 255.f,
                backColor[2] / 255.f
            )
        };
    }
} __attribute__((packed));

struct RvlAnimation {
    uint16_t keyCount;
    uint16_t _pad16 { 0x0000 };

    RvlAnimationKey keys[0];
} __attribute__((packed));

struct CtrPascalString {
    uint8_t stringLength;
    char string[0];
} __attribute__((packed));

struct CtrAnimation {
    uint32_t isInterpolated; // Boolean value.

    uint16_t keyCount;
    uint16_t _pad16 { 0x0000 };

    CtrAnimationKey keys[0];
} __attribute__((packed));

bool CellAnim::CellAnimObject::deserializeImpl_RVL(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(RvlCellAnimHeader)) {
        Logging::error("[CellAnimObject::deserializeImpl_RVL] Invalid RCAD binary: data size smaller than header size!");
        return false;
    }

    const RvlCellAnimHeader* header = reinterpret_cast<const RvlCellAnimHeader*>(data);

    mSheetIndex = BYTESWAP_16(header->sheetIndex);

    mSheetW = BYTESWAP_16(header->sheetW);
    mSheetH = BYTESWAP_16(header->sheetH);

    mUsePalette = header->usePalette != 0x00;

    const unsigned char* currentData = data + sizeof(RvlCellAnimHeader);

    // Arrangements
    const uint16_t arrangementCount = BYTESWAP_16(header->arrangementCount);

    mArrangements.resize(arrangementCount);

    for (unsigned i = 0; i < arrangementCount; i++) {
        const RvlArrangement* arrangementIn = reinterpret_cast<const RvlArrangement*>(currentData);
        currentData += sizeof(RvlArrangement);

        CellAnim::Arrangement& arrangementOut = mArrangements[i];

        const uint16_t arrangementPartCount = BYTESWAP_16(arrangementIn->partsCount);
        arrangementOut.parts.resize(arrangementPartCount);

        for (unsigned j = 0; j < arrangementPartCount; j++) {
            const RvlArrangementPart* arrangementPartIn = reinterpret_cast<const RvlArrangementPart*>(currentData);
            currentData += sizeof(RvlArrangementPart);

            arrangementOut.parts[j] = arrangementPartIn->toArrangementPart();
        }
    }

    const uint16_t animationCount = BYTESWAP_16(
        reinterpret_cast<const AnimationsHeader*>(currentData)->animationCount
    );
    currentData += sizeof(AnimationsHeader);

    mAnimations.resize(animationCount);

    for (unsigned i = 0; i < animationCount; i++) {
        const RvlAnimation* animationIn = reinterpret_cast<const RvlAnimation*>(currentData);
        currentData += sizeof(RvlAnimation);

        CellAnim::Animation& animationOut = mAnimations[i];

        const uint16_t keyCount = BYTESWAP_16(animationIn->keyCount);
        animationOut.keys.resize(keyCount);

        for (unsigned j = 0; j < keyCount; j++) {
            const RvlAnimationKey* keyIn = reinterpret_cast<const RvlAnimationKey*>(currentData);
            currentData += sizeof(RvlAnimationKey);

            animationOut.keys[j] = keyIn->toAnimationKey();
        }
    }

    return true;
}

bool CellAnim::CellAnimObject::deserializeImpl_CTR(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(CtrCellAnimHeader)) {
        Logging::error("[CellAnimObject::deserializeImpl_CTR] Invalid CCAD binary: data size smaller than header size!");
        return false;
    }

    const CtrCellAnimHeader* header = reinterpret_cast<const CtrCellAnimHeader*>(data);

    mSheetIndex = -1;

    mSheetW = header->sheetW;
    mSheetH = header->sheetH;

    mUsePalette = false;

    const unsigned char* currentData = data + sizeof(CtrCellAnimHeader);

    // Arrangements
    mArrangements.resize(header->arrangementCount);

    const unsigned char* arrangementsStart = currentData;

    for (unsigned i = 0; i < header->arrangementCount; i++) {
        const CtrArrangement* arrangementIn = reinterpret_cast<const CtrArrangement*>(currentData);
        currentData += sizeof(CtrArrangement);

        CellAnim::Arrangement& arrangementOut = mArrangements[i];

        arrangementOut.parts.resize(arrangementIn->partsCount);

        for (unsigned j = 0; j < arrangementIn->partsCount; j++) {
            const CtrArrangementPart* arrangementPartIn = reinterpret_cast<const CtrArrangementPart*>(currentData);
            currentData += sizeof(CtrArrangementPart);

            arrangementOut.parts[j] = arrangementPartIn->toArrangementPart();
        }
    }

    const uint16_t animationCount = reinterpret_cast<const AnimationsHeader*>(currentData)->animationCount;
    currentData += sizeof(AnimationsHeader);

    mAnimations.resize(animationCount);

    for (unsigned i = 0; i < animationCount; i++) {
        const CtrPascalString* animationName = reinterpret_cast<const CtrPascalString*>(currentData);
        currentData += ALIGN_UP_4(sizeof(CtrPascalString) + animationName->stringLength + 1);

        const CtrAnimation* animationIn = reinterpret_cast<const CtrAnimation*>(currentData);
        currentData += sizeof(CtrAnimation);

        CellAnim::Animation& animationOut = mAnimations[i];

        animationOut.name.assign(animationName->string, animationName->stringLength);

        animationOut.isInterpolated = animationIn->isInterpolated != 0;

        animationOut.keys.resize(animationIn->keyCount);

        for (unsigned j = 0; j < animationIn->keyCount; j++) {
            const CtrAnimationKey* keyIn = reinterpret_cast<const CtrAnimationKey*>(currentData);
            currentData += sizeof(CtrAnimationKey);

            animationOut.keys[j] = keyIn->toAnimationKey();
        }
    }

    const uint8_t emitterNameCount = *reinterpret_cast<const uint8_t*>(currentData);
    currentData += sizeof(uint8_t);

    if (emitterNameCount != 0) {
        std::vector<std::string> emitterNames(static_cast<unsigned>(emitterNameCount));
        for (unsigned i = 0; i < emitterNameCount; i++) {
            const uint8_t stringLength = *reinterpret_cast<const uint8_t*>(currentData);
            currentData += sizeof(uint8_t);

            emitterNames[i].assign(reinterpret_cast<const char*>(currentData), stringLength);

            currentData += stringLength;
        }

        currentData = arrangementsStart;

        for (unsigned i = 0; i < header->arrangementCount; i++) {
            currentData += sizeof(CtrArrangement);

            CellAnim::Arrangement& arrangement = mArrangements[i];

            for (unsigned j = 0; j < arrangement.parts.size(); j++) {
                const CtrArrangementPart* arrangementPartIn = reinterpret_cast<const CtrArrangementPart*>(currentData);
                currentData += sizeof(CtrArrangementPart);

                if (arrangementPartIn->emitterId != 0xFF)
                    arrangement.parts[j].emitterName = emitterNames[arrangementPartIn->emitterId];
            }
        }
    }

    return true;
}

std::vector<unsigned char> CellAnim::CellAnimObject::serializeImpl_RVL() {
    size_t fullSize = sizeof(RvlCellAnimHeader) + sizeof(AnimationsHeader);
    for (const CellAnim::Arrangement& arrangement : mArrangements)
        fullSize += sizeof(RvlArrangement) + (sizeof(RvlArrangementPart) * arrangement.parts.size());
    for (const CellAnim::Animation& animation : mAnimations)
        fullSize += sizeof(RvlAnimation) + (sizeof(RvlAnimationKey) * animation.keys.size());

    std::vector<unsigned char> result(fullSize);

    RvlCellAnimHeader* header = reinterpret_cast<RvlCellAnimHeader*>(result.data());
    *header = RvlCellAnimHeader {
        .usePalette = static_cast<uint8_t>(mUsePalette ? 0x01 : 0x00),

        .sheetIndex = BYTESWAP_16(static_cast<uint16_t>(mSheetIndex)),

        .sheetW = BYTESWAP_16(static_cast<uint16_t>(mSheetW)),
        .sheetH = BYTESWAP_16(static_cast<uint16_t>(mSheetH)),

        .arrangementCount = BYTESWAP_16(static_cast<uint16_t>(mArrangements.size()))
    };

    unsigned char* currentOutput = result.data() + sizeof(RvlCellAnimHeader);

    for (const CellAnim::Arrangement& arrangement : mArrangements) {
        RvlArrangement* arrangementOut = reinterpret_cast<RvlArrangement*>(currentOutput);
        currentOutput += sizeof(RvlArrangement);

        arrangementOut->partsCount = BYTESWAP_16(static_cast<uint16_t>(arrangement.parts.size()));

        for (const CellAnim::ArrangementPart& part : arrangement.parts) {
            RvlArrangementPart* arrangementPartOut = reinterpret_cast<RvlArrangementPart*>(currentOutput);
            currentOutput += sizeof(RvlArrangementPart);

            *arrangementPartOut = RvlArrangementPart(part);

            if (!part.editorVisible)
                arrangementPartOut->opacity = 0x00;
        }
    }

    *reinterpret_cast<AnimationsHeader*>(currentOutput) = AnimationsHeader {
        .animationCount = BYTESWAP_16(static_cast<uint16_t>(mAnimations.size()))
    };
    currentOutput += sizeof(AnimationsHeader);

    for (const CellAnim::Animation& animation : mAnimations) {
        RvlAnimation* animationRaw = reinterpret_cast<RvlAnimation*>(currentOutput);
        currentOutput += sizeof(RvlAnimation);

        animationRaw->keyCount = BYTESWAP_16(static_cast<uint16_t>(animation.keys.size()));

        for (const CellAnim::AnimationKey& key : animation.keys) {
            RvlAnimationKey* animationKeyOut = reinterpret_cast<RvlAnimationKey*>(currentOutput);
            currentOutput += sizeof(RvlAnimationKey);

            *animationKeyOut = RvlAnimationKey(key);
        }
    }

    return result;
}

std::vector<unsigned char> CellAnim::CellAnimObject::serializeImpl_CTR() {
    std::map<std::string, unsigned> emitterNames;
    unsigned nextEmitterIndex { 0 };

    size_t fullSize = sizeof(CtrCellAnimHeader) + sizeof(AnimationsHeader);
    for (const CellAnim::Arrangement& arrangement : mArrangements) {
        fullSize += sizeof(CtrArrangement);
        for (const CellAnim::ArrangementPart& part : arrangement.parts) {
            if (!part.emitterName.empty()) {
                if (emitterNames.find(part.emitterName) == emitterNames.end())
                    emitterNames[part.emitterName] = nextEmitterIndex++;
            }

            fullSize += sizeof(CtrArrangementPart);
        }
    }
    for (const CellAnim::Animation& animation : mAnimations) {
        fullSize += ALIGN_UP_4(
            sizeof(CtrPascalString) + static_cast<uint8_t>(std::min(animation.name.size(), (size_t)(0xFF))) + 1
        );
        fullSize += sizeof(CtrAnimation) + (sizeof(CtrAnimationKey) * animation.keys.size());
    }

    fullSize += sizeof(uint8_t); // Emitter name count.
    for (const auto& [name, index] : emitterNames)
        fullSize += sizeof(CtrPascalString) + static_cast<uint8_t>(std::min(name.size(), (size_t)(0xFF)));

    std::vector<unsigned char> result(fullSize);

    CtrCellAnimHeader* header = reinterpret_cast<CtrCellAnimHeader*>(result.data());
    *header = CtrCellAnimHeader {
        .sheetW = static_cast<uint16_t>(mSheetW),
        .sheetH = static_cast<uint16_t>(mSheetH),

        .arrangementCount = static_cast<uint16_t>(mArrangements.size())
    };

    unsigned char* currentOutput = result.data() + sizeof(CtrCellAnimHeader);

    for (const CellAnim::Arrangement& arrangement : mArrangements) {
        CtrArrangement* arrangementOut = reinterpret_cast<CtrArrangement*>(currentOutput);
        currentOutput += sizeof(CtrArrangement);

        arrangementOut->partsCount = static_cast<uint16_t>(arrangement.parts.size());

        for (const CellAnim::ArrangementPart& part : arrangement.parts) {
            CtrArrangementPart* arrangementPartOut = reinterpret_cast<CtrArrangementPart*>(currentOutput);
            currentOutput += sizeof(CtrArrangementPart);

            *arrangementPartOut = CtrArrangementPart(part);

            if (!part.emitterName.empty()) {
                auto it = emitterNames.find(part.emitterName);
                if (it != emitterNames.end())
                    arrangementPartOut->emitterId = static_cast<uint8_t>(it->second);
            }

            if (!part.editorVisible)
                arrangementPartOut->opacity = 0x00;
        }
    }

    *reinterpret_cast<AnimationsHeader*>(currentOutput) = AnimationsHeader {
        .animationCount = static_cast<uint16_t>(mAnimations.size())
    };
    currentOutput += sizeof(AnimationsHeader);

    for (const CellAnim::Animation& animation : mAnimations) {
        CtrPascalString* animationNameOut = reinterpret_cast<CtrPascalString*>(currentOutput);

        animationNameOut->stringLength = static_cast<uint8_t>(std::min(animation.name.size(), (size_t)(0xFF)));
        strncpy(animationNameOut->string, animation.name.c_str(), animationNameOut->stringLength + 1);

        currentOutput += ALIGN_UP_4(sizeof(CtrPascalString) + animationNameOut->stringLength + 1);

        CtrAnimation* animationOut = reinterpret_cast<CtrAnimation*>(currentOutput);
        currentOutput += sizeof(CtrAnimation);

        animationOut->isInterpolated = animation.isInterpolated ? 1 : 0;
        animationOut->keyCount = static_cast<uint16_t>(animation.keys.size());

        for (const CellAnim::AnimationKey& key : animation.keys) {
            CtrAnimationKey* animationKeyOut = reinterpret_cast<CtrAnimationKey*>(currentOutput);
            currentOutput += sizeof(CtrAnimationKey);

            *animationKeyOut = CtrAnimationKey(key);
        }
    }

    *reinterpret_cast<uint8_t*>(currentOutput) = static_cast<uint8_t>(
        emitterNames.size()
    );
    currentOutput += sizeof(uint8_t);

    std::vector<const std::string*> sortedEmitters(emitterNames.size());
    for (const auto& [name, index] : emitterNames)
        sortedEmitters[index] = &name;

    for (const std::string* emitterName : sortedEmitters) {
        const uint8_t stringLength = static_cast<uint8_t>(std::min(emitterName->size(), (size_t)(0xFF)));

        *reinterpret_cast<uint8_t*>(currentOutput) = stringLength;
        currentOutput += sizeof(uint8_t);

        strncpy(reinterpret_cast<char*>(currentOutput), emitterName->c_str(), stringLength);
        currentOutput += stringLength;
    }

    return result;
}

namespace CellAnim {

CellAnimObject::CellAnimObject(const unsigned char* data, const size_t dataSize) {
    const uint32_t revisionDate = *reinterpret_cast<const uint32_t*>(data);

    switch (revisionDate) {
    case BYTESWAP_32(RCAD_REVISION_DATE):
        mType = CELLANIM_TYPE_RVL;
        mInitialized = deserializeImpl_RVL(data, dataSize);
        return;
    case CCAD_REVISION_DATE:
        mType = CELLANIM_TYPE_CTR;
        mInitialized = deserializeImpl_CTR(data, dataSize);
        return;

    default:
        Logging::error("[CellAnimObject::CellAnimObject] Invalid cellanim binary: revision date doesn't match RVL or CTR!");
        return;
    }
}

std::vector<unsigned char> CellAnimObject::Serialize() {
    switch (mType) {
    case CELLANIM_TYPE_RVL:
        return serializeImpl_RVL();
    case CELLANIM_TYPE_CTR:
        return serializeImpl_CTR();

    default:
        Logging::error("[CellAnimObject::Serialize] Invalid type ({})!", static_cast<int>(mType));
        return std::vector<unsigned char>();
    }
}

size_t CellAnimObject::countArrangementUses(unsigned arrangementIndex) const {
    size_t usageCount = 0;
    for (const auto &animation : mAnimations) {
        for (const auto &key : animation.keys) {
            if (key.arrangementIndex == arrangementIndex)
                usageCount++;
        }
    }
    return usageCount;
}

std::vector<Arrangement>::iterator CellAnimObject::insertArrangement(const Arrangement& arrangement) {
    mArrangements.push_back(arrangement);
    return std::prev(mArrangements.end());
}

} // namespace CellAnim
