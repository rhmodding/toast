#include "RvlCellAnim.hpp"

#include <cstring>

#include <iostream>
#include <fstream>

#include "../common.hpp"

// Pre-byteswapped
#define RCAD_REVISION_DATE (0xD8B43201)

struct RvlCellAnimHeader {
    // Format revision date (should equal 20100312 in BE (2010/03/12) if valid)
    // Compare to RCAD_REVISION_DATE
    uint32_t revisionDate;

    // Load textures in palette (CI) mode (boolean)
    uint8_t usePalette;

    uint8_t _pad24[3] { 0x00, 0x00, 0x00 };

    // Index into cellanim.tpl for the associated sheet
    uint16_t sheetIndex;

    // Unused value. Never referenced in the game
    uint16_t _unused { 0x0000 };

    uint16_t sheetW; // Sheet width in relation to UV regions
    uint16_t sheetH; // Sheet height in relation to UV regions
} __attribute__((packed));

struct ArrangementsHeader {
    // Amount of arrangements (commonly referred to as sprites)
    uint16_t arrangementCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

struct TransformValuesRaw {
    int16_t positionX { 0 }, positionY { 0 };
    float scaleX { 1.f }, scaleY { 1.f };
    float angle { 0.f }; // Angle in degrees

    TransformValuesRaw() = default;
    TransformValuesRaw(const RvlCellAnim::TransformValues& transformValues, bool isArrangementPart) {
        const unsigned additive = (isArrangementPart ? 512 : 0);

        this->positionX = BYTESWAP_16(static_cast<int16_t>(transformValues.positionX + additive));
        this->positionY = BYTESWAP_16(static_cast<int16_t>(transformValues.positionY + additive));
        this->scaleX = Common::byteswapFloat(transformValues.scaleX);
        this->scaleY = Common::byteswapFloat(transformValues.scaleY);
        this->angle = Common::byteswapFloat(transformValues.angle);
    }

    RvlCellAnim::TransformValues toTransformValues(bool isArrangementPart) const {
        RvlCellAnim::TransformValues transformValues;

        const unsigned additive = (isArrangementPart ? -512 : 0);

        transformValues.positionX = BYTESWAP_16(this->positionX) + additive;
        transformValues.positionY = BYTESWAP_16(this->positionY) + additive;
        transformValues.scaleX = Common::byteswapFloat(this->scaleX);
        transformValues.scaleY = Common::byteswapFloat(this->scaleY);
        transformValues.angle = Common::byteswapFloat(this->angle);

        return transformValues;
    }
} __attribute__((packed));

struct ArrangementPartRaw {
    uint16_t regionX; // X position of UV region in spritesheet
    uint16_t regionY; // Y position of UV region in spritesheet
    uint16_t regionW; // Width of UV region in spritesheet
    uint16_t regionH; // Height of UV region in spritesheet

    uint16_t textureVarying; // Additive to the texture index if usePalette is true

    uint16_t _pad16 { 0x0000 };

    TransformValuesRaw transform;

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

    uint8_t opacity;

    uint8_t _pad8 { 0x00 };

    ArrangementPartRaw() = default;
    ArrangementPartRaw(const RvlCellAnim::ArrangementPart& arrangementPart) {
        this->regionX = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionX));
        this->regionY = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionY));
        this->regionW = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionW));
        this->regionH = BYTESWAP_16(static_cast<uint16_t>(arrangementPart.regionH));

        this->textureVarying = BYTESWAP_16(arrangementPart.textureVarying);

        this->transform = TransformValuesRaw(arrangementPart.transform, true);

        this->flipX = arrangementPart.flipX ? 1 : 0;
        this->flipY = arrangementPart.flipY ? 1 : 0;

        this->opacity = arrangementPart.opacity;
    }
} __attribute__((packed));

struct ArrangementRaw {
    // Amout of parts in the arrangement (commonly referred to as a sprite)
    uint16_t partsCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

struct AnimationsHeader {
    // Amount of animations
    uint16_t animationCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

struct AnimationRaw {
    // Amount of keys the animation has
    uint16_t keyCount;

    uint16_t _pad16 { 0x0000 };
} __attribute__((packed));

struct AnimationKeyRaw {
    // Index of arrangement (commonly referred to as sprites)
    uint16_t arrangementIndex;

    // Amount of frames the key is held for
    uint16_t holdFrames;

    TransformValuesRaw transform;

    uint8_t opacity;

    uint8_t _pad24[3] { 0x00, 0x00, 0x00 };

    AnimationKeyRaw() = default;
    AnimationKeyRaw(const RvlCellAnim::AnimationKey& animationKey) {
        this->arrangementIndex = BYTESWAP_16(animationKey.arrangementIndex);
        this->holdFrames = BYTESWAP_16(animationKey.holdFrames);

        this->transform = TransformValuesRaw(animationKey.transform, false);

        this->opacity = animationKey.opacity;
    }
} __attribute__((packed));

namespace RvlCellAnim {

RvlCellAnimObject::RvlCellAnimObject(const unsigned char* RvlCellAnimData, const size_t dataSize) {
    const RvlCellAnimHeader* header = reinterpret_cast<const RvlCellAnimHeader*>(RvlCellAnimData);

    if (header->revisionDate != RCAD_REVISION_DATE) {
        std::cerr << "[RvlCellAnimObject::RvlCellAnimObject] Invalid RvlCellAnim binary: revision date failed check!\n";
        return;
    }

    this->sheetIndex = BYTESWAP_16(header->sheetIndex);

    this->textureW = BYTESWAP_16(header->sheetW);
    this->textureH = BYTESWAP_16(header->sheetH);

    this->usePalette = header->usePalette != 0x00;

    unsigned readOffset { sizeof(RvlCellAnimHeader) };

    // Arrangements
    const uint16_t arrangementCount = BYTESWAP_16(
        reinterpret_cast<const ArrangementsHeader*>(
            RvlCellAnimData + readOffset
        )->arrangementCount
    );
    readOffset += sizeof(ArrangementsHeader);

    this->arrangements.resize(arrangementCount);

    for (unsigned i = 0; i < arrangementCount; i++) {
        const ArrangementRaw* arrangementRaw =
            reinterpret_cast<const ArrangementRaw*>(RvlCellAnimData + readOffset);
        readOffset += sizeof(ArrangementRaw);

        Arrangement& arrangement = this->arrangements[i];

        const uint16_t arrangementPartCount = BYTESWAP_16(arrangementRaw->partsCount);
        arrangement.parts.resize(arrangementPartCount);

        for (unsigned j = 0; j < arrangementPartCount; j++) {
            const ArrangementPartRaw* arrangementPartRaw =
                reinterpret_cast<const ArrangementPartRaw*>(RvlCellAnimData + readOffset);
            readOffset += sizeof(ArrangementPartRaw);

            arrangement.parts[j] = ArrangementPart {
                .regionX = BYTESWAP_16(arrangementPartRaw->regionX),
                .regionY = BYTESWAP_16(arrangementPartRaw->regionY),
                .regionW = BYTESWAP_16(arrangementPartRaw->regionW),
                .regionH = BYTESWAP_16(arrangementPartRaw->regionH),

                .textureVarying = BYTESWAP_16(arrangementPartRaw->textureVarying),

                .transform = arrangementPartRaw->transform.toTransformValues(true),

                .flipX = arrangementPartRaw->flipX != 0x00,
                .flipY = arrangementPartRaw->flipY != 0x00,

                .opacity = arrangementPartRaw->opacity
            };
        }
    }

    const uint16_t animationCount = BYTESWAP_16(
        reinterpret_cast<const AnimationsHeader*>(
            RvlCellAnimData + readOffset
        )->animationCount
    );
    readOffset += sizeof(AnimationsHeader);

    this->animations.resize(animationCount);

    for (unsigned i = 0; i < animationCount; i++) {
        const AnimationRaw* animationRaw = reinterpret_cast<const AnimationRaw*>(RvlCellAnimData + readOffset);
        readOffset += sizeof(AnimationRaw);

        Animation& animation = this->animations[i];

        const uint16_t keyCount = BYTESWAP_16(animationRaw->keyCount);
        animation.keys.resize(keyCount);

        for (unsigned j = 0; j < keyCount; j++) {
            const AnimationKeyRaw* keyRaw = reinterpret_cast<const AnimationKeyRaw*>(RvlCellAnimData + readOffset);
            readOffset += sizeof(AnimationKeyRaw);

            animation.keys[j] = AnimationKey {
                .arrangementIndex = BYTESWAP_16(keyRaw->arrangementIndex),

                .holdFrames = BYTESWAP_16(keyRaw->holdFrames),

                .transform = keyRaw->transform.toTransformValues(false),

                .opacity = keyRaw->opacity
            };
        }
    }

    this->ok = true;
}

std::vector<unsigned char> RvlCellAnimObject::Reserialize() {
    // Allocate persistent size
    std::vector<unsigned char> result(
        sizeof(RvlCellAnimHeader) +
        sizeof(ArrangementsHeader) +
        sizeof(AnimationsHeader)
    );

    RvlCellAnimHeader* header = reinterpret_cast<RvlCellAnimHeader*>(result.data());
    header->revisionDate = RCAD_REVISION_DATE;

    header->usePalette = this->usePalette ? 0x01 : 0x00;

    header->sheetIndex = BYTESWAP_16(this->sheetIndex);

    header->sheetW = BYTESWAP_16(this->textureW);
    header->sheetH = BYTESWAP_16(this->textureH);

    unsigned writeOffset { sizeof(RvlCellAnimHeader) };

    *reinterpret_cast<ArrangementsHeader*>(result.data() + writeOffset) =
        ArrangementsHeader { BYTESWAP_16(static_cast<uint16_t>(this->arrangements.size())) };
    writeOffset += sizeof(ArrangementsHeader);

    for (const Arrangement& arrangement : this->arrangements) {
        result.resize(
            result.size() +
            sizeof(ArrangementRaw) +
            (sizeof(ArrangementPartRaw) * arrangement.parts.size())
        );

        ArrangementRaw* arrangementRaw = reinterpret_cast<ArrangementRaw*>(result.data() + writeOffset);
        writeOffset += sizeof(ArrangementRaw);

        arrangementRaw->partsCount = BYTESWAP_16(static_cast<uint16_t>(arrangement.parts.size()));

        for (const ArrangementPart& part : arrangement.parts) {
            ArrangementPartRaw* arrangementPartRaw =
                reinterpret_cast<ArrangementPartRaw*>(result.data() + writeOffset);
            writeOffset += sizeof(ArrangementPartRaw);

            *arrangementPartRaw = ArrangementPartRaw(part);

            if (!part.editorVisible)
                arrangementPartRaw->opacity = 0x00;
        }
    }

    *reinterpret_cast<AnimationsHeader*>(result.data() + writeOffset) =
        AnimationsHeader { BYTESWAP_16(static_cast<uint16_t>(this->animations.size())) };
    writeOffset += sizeof(AnimationsHeader);

    for (const Animation& animation : this->animations) {
        result.resize(
            result.size() +
            sizeof(AnimationRaw) +
            (sizeof(AnimationKeyRaw) * animation.keys.size())
        );

        AnimationRaw* animationRaw = reinterpret_cast<AnimationRaw*>(result.data() + writeOffset);
        writeOffset += sizeof(AnimationRaw);

        animationRaw->keyCount = BYTESWAP_16(static_cast<uint16_t>(animation.keys.size()));

        for (const AnimationKey& key : animation.keys) {
            AnimationKeyRaw* animationKeyRaw =
                reinterpret_cast<AnimationKeyRaw*>(result.data() + writeOffset);
            writeOffset += sizeof(AnimationKeyRaw);

            *animationKeyRaw = AnimationKeyRaw(key);
        }
    }

    return result;
}

std::shared_ptr<RvlCellAnimObject> readRvlCellAnimFile(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[RvlCellAnim::readRvlCellAnimFile] Could not open file at path: " << filePath << '\n';
        return nullptr;
    }

    const std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    unsigned char* buffer = new unsigned char[fileSize];
    file.read(reinterpret_cast<char*>(buffer), fileSize);

    file.close();

    std::shared_ptr<RvlCellAnimObject> object =
        std::make_shared<RvlCellAnimObject>(buffer, fileSize);

    delete[] buffer;

    return object;
}

} // namespace RvlCellAnim
