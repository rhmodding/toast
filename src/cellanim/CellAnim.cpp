#include "CellAnim.hpp"

#include <cstring>

#include <map>

#include "../Logging.hpp"

#include "../common.hpp"

// 12 Mar 2010
// Pre-byteswapped to BE.
constexpr uint32_t RCAD_REVISION_DATE = BYTESWAP_32(20100312);

// 7 Oct 2013
constexpr uint32_t CCAD_REVISION_DATE = 20131007;

struct RvlCellAnimHeader {
    // Compare to RCAD_REVISION_DATE
    uint32_t revisionDate { RCAD_REVISION_DATE };

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
    // Compare to CCAD_REVISION_DATE
    uint32_t revisionDate { CCAD_REVISION_DATE };

    uint16_t sheetW; // Sheet width in relation to UV regions.
    uint16_t sheetH; // Sheet height in relation to UV regions.

    uint16_t arrangementCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

struct RvlTransformValues {
    int16_t positionX { 0 }, positionY { 0 }; // In pixels.
    float scaleX { 1.f }, scaleY { 1.f };
    float angle { 0.f }; // In degrees.

    RvlTransformValues() = default;
    RvlTransformValues(const CellAnim::TransformValues& transformValues, bool isArrangementPart) {
        const int additive = (isArrangementPart ? 512 : 0);

        this->positionX = BYTESWAP_16(static_cast<int16_t>(transformValues.positionX + additive));
        this->positionY = BYTESWAP_16(static_cast<int16_t>(transformValues.positionY + additive));
        this->scaleX = BYTESWAP_FLOAT(transformValues.scaleX);
        this->scaleY = BYTESWAP_FLOAT(transformValues.scaleY);
        this->angle = BYTESWAP_FLOAT(transformValues.angle);
    }

    CellAnim::TransformValues toTransformValues(bool isArrangementPart) const {
        CellAnim::TransformValues transformValues;

        const int additive = (isArrangementPart ? -512 : 0);

        transformValues.positionX = static_cast<int>(static_cast<int16_t>(BYTESWAP_16(this->positionX) + additive));
        transformValues.positionY = static_cast<int>(static_cast<int16_t>(BYTESWAP_16(this->positionY) + additive));
        transformValues.scaleX = BYTESWAP_FLOAT(this->scaleX);
        transformValues.scaleY = BYTESWAP_FLOAT(this->scaleY);
        transformValues.angle = BYTESWAP_FLOAT(this->angle);

        return transformValues;
    }
} __attribute__((packed));

// Note: In Bread (the predecessor of toast) 'arrangements' were called 'sprites'.

struct RvlArrangementPart {
    uint16_t regionX; // X position of UV region in spritesheet
    uint16_t regionY; // Y position of UV region in spritesheet
    uint16_t regionW; // Width of UV region in spritesheet
    uint16_t regionH; // Height of UV region in spritesheet

    // Additive to the texture index (if usePalette is true).
    uint16_t textureVarying;

    uint16_t _pad16 { 0x0000 };

    RvlTransformValues transform;

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

    uint8_t opacity;

    uint8_t _pad8 { 0x00 };

    RvlArrangementPart() = default;
    RvlArrangementPart(const CellAnim::ArrangementPart& arrangementPart) {
        this->regionX = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionX));
        this->regionY = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionY));
        this->regionW = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionW));
        this->regionH = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionH));

        this->textureVarying = BYTESWAP_16(arrangementPart.textureVarying);

        this->transform = RvlTransformValues(arrangementPart.transform, true);

        this->flipX = arrangementPart.flipX ? 0x01 : 0x00;
        this->flipY = arrangementPart.flipY ? 0x01 : 0x00;

        this->opacity = arrangementPart.opacity;
    }
} __attribute__((packed));
struct CtrArrangementPart {
    uint16_t regionX; // X position of UV region in spritesheet
    uint16_t regionY; // Y position of UV region in spritesheet
    uint16_t regionW; // Width of UV region in spritesheet
    uint16_t regionH; // Height of UV region in spritesheet

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
        this->regionX = arrangementPart.regionX;
        this->regionY = arrangementPart.regionY;
        this->regionW = arrangementPart.regionW;
        this->regionH = arrangementPart.regionH;

        this->positionX = static_cast<int16_t>(arrangementPart.transform.positionX + 512);
        this->positionY = static_cast<int16_t>(arrangementPart.transform.positionY + 512);
        this->scaleX = arrangementPart.transform.scaleX;
        this->scaleY = arrangementPart.transform.scaleY;
        this->angle = arrangementPart.transform.angle;

        this->flipX = arrangementPart.flipX ? 0x01 : 0x00;
        this->flipY = arrangementPart.flipY ? 0x01 : 0x00;

        this->foreColor[0] = ROUND_FLOAT(arrangementPart.foreColor.r * 255.f);
        this->foreColor[1] = ROUND_FLOAT(arrangementPart.foreColor.g * 255.f);
        this->foreColor[2] = ROUND_FLOAT(arrangementPart.foreColor.b * 255.f);
        this->backColor[0] = ROUND_FLOAT(arrangementPart.backColor.r * 255.f);
        this->backColor[1] = ROUND_FLOAT(arrangementPart.backColor.g * 255.f);
        this->backColor[2] = ROUND_FLOAT(arrangementPart.backColor.b * 255.f);

        this->opacity = arrangementPart.opacity;

        memset(this->_unknown0, 0xFF, 12);

        this->id = arrangementPart.id;

        // this->emitterId = arrangementPart.emitterId;

        this->depthTL = arrangementPart.quadDepth.topLeft;
        this->depthBL = arrangementPart.quadDepth.bottomLeft;
        this->depthTR = arrangementPart.quadDepth.topRight;
        this->depthBR = arrangementPart.quadDepth.bottomRight;
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

    // Amount of frames the key is held for (0 is invalid).
    uint16_t holdFrames;

    RvlTransformValues transform;

    uint8_t opacity;

    uint8_t _pad24[3] { 0x00, 0x00, 0x00 };

    RvlAnimationKey() = default;
    RvlAnimationKey(const CellAnim::AnimationKey& animationKey) {
        this->arrangementIndex = BYTESWAP_16(animationKey.arrangementIndex);
        this->holdFrames = BYTESWAP_16(animationKey.holdFrames);

        this->transform = RvlTransformValues(animationKey.transform, false);

        this->opacity = animationKey.opacity;
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
        this->arrangementIndex = animationKey.arrangementIndex;
        this->holdFrames = animationKey.holdFrames;

        this->positionX = animationKey.transform.positionX;
        this->positionY = animationKey.transform.positionY;

        this->positionZ = animationKey.translateZ;

        this->scaleX = animationKey.transform.scaleX;
        this->scaleY = animationKey.transform.scaleY;
        this->angle = animationKey.transform.angle;

        this->foreColor[0] = ROUND_FLOAT(animationKey.foreColor.r * 255.f);
        this->foreColor[1] = ROUND_FLOAT(animationKey.foreColor.g * 255.f);
        this->foreColor[2] = ROUND_FLOAT(animationKey.foreColor.b * 255.f);
        this->backColor[0] = ROUND_FLOAT(animationKey.backColor.r * 255.f);
        this->backColor[1] = ROUND_FLOAT(animationKey.backColor.g * 255.f);
        this->backColor[2] = ROUND_FLOAT(animationKey.backColor.b * 255.f);

        this->opacity = animationKey.opacity;
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

static bool InitRvlCellAnim(
    CellAnim::CellAnimObject& object,
    const unsigned char* data, const size_t dataSize
) {
    if (dataSize < sizeof(RvlCellAnimHeader)) {
        Logging::err << "[CellAnimObject::CellAnimObject] Invalid RCAD binary: data size smaller than header size!" << std::endl;
        return false;
    }

    const RvlCellAnimHeader* header = reinterpret_cast<const RvlCellAnimHeader*>(data);

    object.sheetIndex = BYTESWAP_16(header->sheetIndex);

    object.sheetW = BYTESWAP_16(header->sheetW);
    object.sheetH = BYTESWAP_16(header->sheetH);

    object.usePalette = header->usePalette != 0x00;

    const unsigned char* currentData = data + sizeof(RvlCellAnimHeader);

    // Arrangements
    const uint16_t arrangementCount = BYTESWAP_16(header->arrangementCount);

    object.arrangements.resize(arrangementCount);

    for (unsigned i = 0; i < arrangementCount; i++) {
        const RvlArrangement* arrangementIn = reinterpret_cast<const RvlArrangement*>(currentData);
        currentData += sizeof(RvlArrangement);

        CellAnim::Arrangement& arrangementOut = object.arrangements[i];

        const uint16_t arrangementPartCount = BYTESWAP_16(arrangementIn->partsCount);
        arrangementOut.parts.resize(arrangementPartCount);

        for (unsigned j = 0; j < arrangementPartCount; j++) {
            const RvlArrangementPart* arrangementPartIn = reinterpret_cast<const RvlArrangementPart*>(currentData);
            currentData += sizeof(RvlArrangementPart);

            arrangementOut.parts[j] = CellAnim::ArrangementPart {
                .regionX = BYTESWAP_16(arrangementPartIn->regionX),
                .regionY = BYTESWAP_16(arrangementPartIn->regionY),
                .regionW = BYTESWAP_16(arrangementPartIn->regionW),
                .regionH = BYTESWAP_16(arrangementPartIn->regionH),

                .textureVarying = BYTESWAP_16(arrangementPartIn->textureVarying),

                .transform = arrangementPartIn->transform.toTransformValues(true),

                .flipX = arrangementPartIn->flipX != 0x00,
                .flipY = arrangementPartIn->flipY != 0x00,

                .opacity = arrangementPartIn->opacity
            };
        }
    }

    const uint16_t animationCount = BYTESWAP_16(
        reinterpret_cast<const AnimationsHeader*>(currentData)->animationCount
    );
    currentData += sizeof(AnimationsHeader);

    object.animations.resize(animationCount);

    for (unsigned i = 0; i < animationCount; i++) {
        const RvlAnimation* animationIn = reinterpret_cast<const RvlAnimation*>(currentData);
        currentData += sizeof(RvlAnimation);

        CellAnim::Animation& animationOut = object.animations[i];

        const uint16_t keyCount = BYTESWAP_16(animationIn->keyCount);
        animationOut.keys.resize(keyCount);

        for (unsigned j = 0; j < keyCount; j++) {
            const RvlAnimationKey* keyIn = reinterpret_cast<const RvlAnimationKey*>(currentData);
            currentData += sizeof(RvlAnimationKey);

            animationOut.keys[j] = CellAnim::AnimationKey {
                .arrangementIndex = BYTESWAP_16(keyIn->arrangementIndex),

                .holdFrames = BYTESWAP_16(keyIn->holdFrames),

                .transform = keyIn->transform.toTransformValues(false),

                .opacity = keyIn->opacity
            };
        }
    }

    return true;
}

static bool InitCtrCellAnim(
    CellAnim::CellAnimObject& object,
    const unsigned char* data, const size_t dataSize
) {
    if (dataSize < sizeof(CtrCellAnimHeader)) {
        Logging::err << "[CellAnimObject::CellAnimObject] Invalid CCAD binary: data size smaller than header size!" << std::endl;
        return false;
    }

    const CtrCellAnimHeader* header = reinterpret_cast<const CtrCellAnimHeader*>(data);

    object.sheetW = header->sheetW;
    object.sheetH = header->sheetH;

    const unsigned char* currentData = data + sizeof(CtrCellAnimHeader);

    // Arrangements
    object.arrangements.resize(header->arrangementCount);

    const unsigned char* arrangementsStart = currentData;

    for (unsigned i = 0; i < header->arrangementCount; i++) {
        const CtrArrangement* arrangementIn = reinterpret_cast<const CtrArrangement*>(currentData);
        currentData += sizeof(CtrArrangement);

        CellAnim::Arrangement& arrangementOut = object.arrangements[i];

        arrangementOut.parts.resize(arrangementIn->partsCount);

        for (unsigned j = 0; j < arrangementIn->partsCount; j++) {
            const CtrArrangementPart* arrangementPartIn = reinterpret_cast<const CtrArrangementPart*>(currentData);
            currentData += sizeof(CtrArrangementPart);

            arrangementOut.parts[j] = CellAnim::ArrangementPart {
                .regionX = arrangementPartIn->regionX,
                .regionY = arrangementPartIn->regionY,
                .regionW = arrangementPartIn->regionW,
                .regionH = arrangementPartIn->regionH,

                .transform = CellAnim::TransformValues {
                    .positionX = static_cast<int>(static_cast<int16_t>(arrangementPartIn->positionX - 512)),
                    .positionY = static_cast<int>(static_cast<int16_t>(arrangementPartIn->positionY - 512)),
                    .scaleX = arrangementPartIn->scaleX,
                    .scaleY = arrangementPartIn->scaleY,
                    .angle = arrangementPartIn->angle,
                },

                .flipX = arrangementPartIn->flipX != 0x00,
                .flipY = arrangementPartIn->flipY != 0x00,

                .opacity = arrangementPartIn->opacity,

                .foreColor = CellAnim::CTRColor {
                    .r = arrangementPartIn->foreColor[0] / 255.f,
                    .g = arrangementPartIn->foreColor[1] / 255.f,
                    .b = arrangementPartIn->foreColor[2] / 255.f,
                },
                .backColor = CellAnim::CTRColor {
                    .r = arrangementPartIn->backColor[0] / 255.f,
                    .g = arrangementPartIn->backColor[1] / 255.f,
                    .b = arrangementPartIn->backColor[2] / 255.f,
                },

                .quadDepth = CellAnim::CTRQuadDepth {
                    .topLeft = arrangementPartIn->depthTL,
                    .bottomLeft = arrangementPartIn->depthBL,
                    .topRight = arrangementPartIn->depthTR,
                    .bottomRight = arrangementPartIn->depthBR,
                },

                .id = arrangementPartIn->id

                // emitterName is set later.
            };
        }
    }

    const uint16_t animationCount = reinterpret_cast<const AnimationsHeader*>(currentData)->animationCount;
    currentData += sizeof(AnimationsHeader);

    object.animations.resize(animationCount);

    for (unsigned i = 0; i < animationCount; i++) {
        const CtrPascalString* animationName = reinterpret_cast<const CtrPascalString*>(currentData);
        currentData += ALIGN_UP_4(sizeof(CtrPascalString) + animationName->stringLength + 1);

        const CtrAnimation* animationIn = reinterpret_cast<const CtrAnimation*>(currentData);
        currentData += sizeof(CtrAnimation);

        CellAnim::Animation& animationOut = object.animations[i];

        animationOut.name.assign(animationName->string, animationName->stringLength);

        animationOut.isInterpolated = animationIn->isInterpolated != 0;

        animationOut.keys.resize(animationIn->keyCount);

        for (unsigned j = 0; j < animationIn->keyCount; j++) {
            const CtrAnimationKey* keyIn = reinterpret_cast<const CtrAnimationKey*>(currentData);
            currentData += sizeof(CtrAnimationKey);

            animationOut.keys[j] = CellAnim::AnimationKey {
                .arrangementIndex = keyIn->arrangementIndex,

                .holdFrames = keyIn->holdFrames,

                .transform = CellAnim::TransformValues {
                    .positionX = keyIn->positionX,
                    .positionY = keyIn->positionY,

                    .scaleX = keyIn->scaleX,
                    .scaleY = keyIn->scaleY,
                    .angle = keyIn->angle,
                },

                .translateZ = keyIn->positionZ,

                .opacity = keyIn->opacity,

                .foreColor = CellAnim::CTRColor {
                    .r = keyIn->foreColor[0] / 255.f,
                    .g = keyIn->foreColor[1] / 255.f,
                    .b = keyIn->foreColor[2] / 255.f,
                },
                .backColor = CellAnim::CTRColor {
                    .r = keyIn->backColor[0] / 255.f,
                    .g = keyIn->backColor[1] / 255.f,
                    .b = keyIn->backColor[2] / 255.f,
                }
            };
        }
    }

    const uint8_t emitterNameCount = *reinterpret_cast<const uint8_t*>(currentData);
    currentData += sizeof(uint8_t);

    if (emitterNameCount != 0) {
        std::vector<std::string> emitterNames(static_cast<unsigned>(emitterNameCount));
        for (unsigned i = 0; i < emitterNameCount; i++) {
            const uint8_t stringLength = *reinterpret_cast<const uint8_t*>(currentData);

            emitterNames[i].assign(reinterpret_cast<const char*>(currentData + 1), stringLength);

            currentData += sizeof(uint8_t) + stringLength;
        }

        currentData = arrangementsStart;

        for (unsigned i = 0; i < header->arrangementCount; i++) {
            currentData += sizeof(CtrArrangement);
    
            CellAnim::Arrangement& arrangement = object.arrangements[i];
    
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

static std::vector<unsigned char> SerializeRvlCellAnim(
    const CellAnim::CellAnimObject& object
) {
    unsigned fullSize = sizeof(RvlCellAnimHeader) + sizeof(AnimationsHeader);
    for (const CellAnim::Arrangement& arrangement : object.arrangements)
        fullSize += sizeof(RvlArrangement) + (sizeof(RvlArrangementPart) * arrangement.parts.size());
    for (const CellAnim::Animation& animation : object.animations)
        fullSize += sizeof(RvlAnimation) + (sizeof(RvlAnimationKey) * animation.keys.size());

    std::vector<unsigned char> result(fullSize);

    RvlCellAnimHeader* header = reinterpret_cast<RvlCellAnimHeader*>(result.data());
    *header = RvlCellAnimHeader {
        .usePalette = static_cast<uint8_t>(object.usePalette ? 0x01 : 0x00),

        .sheetIndex = BYTESWAP_16(static_cast<uint16_t>(object.sheetIndex)),

        .sheetW = BYTESWAP_16(static_cast<uint16_t>(object.sheetW)),
        .sheetH = BYTESWAP_16(static_cast<uint16_t>(object.sheetH)),

        .arrangementCount = BYTESWAP_16(static_cast<uint16_t>(object.arrangements.size()))
    };

    unsigned char* currentOutput = result.data() + sizeof(RvlCellAnimHeader);

    for (const CellAnim::Arrangement& arrangement : object.arrangements) {
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
        .animationCount = BYTESWAP_16(static_cast<uint16_t>(object.animations.size()))
    };
    currentOutput += sizeof(AnimationsHeader);

    for (const CellAnim::Animation& animation : object.animations) {
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

static std::vector<unsigned char> SerializeCtrCellAnim(
    const CellAnim::CellAnimObject& object
) {
    std::map<std::string, unsigned> emitterNames;
    unsigned nextEmitterIndex { 0 };

    unsigned fullSize = sizeof(CtrCellAnimHeader) + sizeof(AnimationsHeader);
    for (const CellAnim::Arrangement& arrangement : object.arrangements) {
        fullSize += sizeof(CtrArrangement);
        for (const CellAnim::ArrangementPart& part : arrangement.parts) {
            if (!part.emitterName.empty()) {
                if (emitterNames.find(part.emitterName) == emitterNames.end())
                    emitterNames[part.emitterName] = nextEmitterIndex++;
            }

            fullSize += sizeof(CtrArrangementPart);
        }
    }
    for (const CellAnim::Animation& animation : object.animations) {
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
        .sheetW = static_cast<uint16_t>(object.sheetW),
        .sheetH = static_cast<uint16_t>(object.sheetH),

        .arrangementCount = static_cast<uint16_t>(object.arrangements.size())
    };

    unsigned char* currentOutput = result.data() + sizeof(CtrCellAnimHeader);

    for (const CellAnim::Arrangement& arrangement : object.arrangements) {
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
        .animationCount = static_cast<uint16_t>(object.animations.size())
    };
    currentOutput += sizeof(AnimationsHeader);

    for (const CellAnim::Animation& animation : object.animations) {
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

        strncpy(reinterpret_cast<char*>(currentOutput + 1), emitterName->c_str(), stringLength);
        currentOutput += sizeof(uint8_t) + stringLength;
    }

    return result;
}

namespace CellAnim {

CellAnimObject::CellAnimObject(const unsigned char* data, const size_t dataSize) {
    const uint32_t revisionDate = *reinterpret_cast<const uint32_t*>(data);

    switch (revisionDate) {
    case RCAD_REVISION_DATE:
        this->type = CELLANIM_TYPE_RVL;
        this->initialized = InitRvlCellAnim(*this, data, dataSize);
        return;
    case CCAD_REVISION_DATE:
        this->type = CELLANIM_TYPE_CTR;
        this->initialized = InitCtrCellAnim(*this, data, dataSize);
        return;

    default:
        Logging::err << "[CellAnimObject::CellAnimObject] Invalid cellanim binary: revision date doesn't match RVL or CTR!" << std::endl;
        return;
    }
}

std::vector<unsigned char> CellAnimObject::Serialize() {
    switch (this->type) {
    case CELLANIM_TYPE_RVL:
        return SerializeRvlCellAnim(*this);
    case CELLANIM_TYPE_CTR:
        return SerializeCtrCellAnim(*this);

    default:
        Logging::err << "[CellAnimObject::Serialize] Invalid type (" <<
            static_cast<int>(this->type) << ")!" << std::endl;
        return std::vector<unsigned char>();
    }
}

} // namespace CellAnim
