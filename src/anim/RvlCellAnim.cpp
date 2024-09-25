#include "RvlCellAnim.hpp"

#include <cstring>

#include <iostream>
#include <fstream>

#include "../common.hpp"

#define RCAD_REVISION_DATE 0xD8B43201

struct RvlCellAnimHeader {
    // Format revision date, 20100312 in BE (2010/03/12)
    uint32_t revisionDate;

    // TPL has palette usage (boolean)
    uint32_t usePalette;

    // Index into cellanim.tpl for the associated sheet
    uint16_t sheetIndex;

    // Used for debugging purposes in the game, always 0x0000
    uint16_t _unused{ 0x0000 };

    uint16_t sheetW; // Sheet width in relation to UV regions
    uint16_t sheetH; // Sheet height in relation to UV regions
} __attribute__((packed));

struct ArrangementsHeader {
    // Amount of arrangements (commonly referred to as sprites)
    uint16_t arrangementCount;

    uint16_t _pad16{ 0x0000 };
} __attribute__((packed));

// Same structure
typedef RvlCellAnim::TransformValues TransformValuesRaw;

struct ArrangementPartRaw {
    uint16_t regionX; // X position of UV region in spritesheet
    uint16_t regionY; // Y position of UV region in spritesheet
    uint16_t regionW; // Width of UV region in spritesheet
    uint16_t regionH; // Height of UV region in spritesheet

    uint16_t _reserved{ 0x0000 }; // Palette index, set by game

    uint16_t _pad16{ 0x0000 };

    TransformValuesRaw transform;

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

    uint8_t opacity;

    uint8_t _pad8{ 0x00 };
} __attribute__((packed));

struct ArrangementRaw {
    // Amout of parts in the arrangement (commonly referred to as a sprite)
    uint16_t partsCount;

    uint16_t _pad16{ 0x0000 };
} __attribute__((packed));

struct AnimationsHeader {
    // Amount of animations
    uint16_t animationCount;

    uint16_t _pad16{ 0x0000 };
} __attribute__((packed));

struct AnimationRaw {
    // Amount of keys the animation has
    uint16_t keyCount;

    uint16_t _pad16{ 0x0000 };
} __attribute__((packed));

struct AnimationKeyRaw {
    // Index of arrangement (commonly referred to as sprites)
    uint16_t arrangementIndex;

    // Amount of frames the key is held for
    uint16_t holdFrames;

    TransformValuesRaw transform;

    uint8_t opacity;

    uint8_t _pad24[3]{ 0x00, 0x00, 0x00 };
} __attribute__((packed));

#define EXPECT_DATA_FOOTER "EXPECTDT"

namespace RvlCellAnim {

RvlCellAnimObject::RvlCellAnimObject(const unsigned char* RvlCellAnimData, const size_t dataSize) {
    const RvlCellAnimHeader* header = reinterpret_cast<const RvlCellAnimHeader*>(RvlCellAnimData);

    if (UNLIKELY(header->revisionDate != RCAD_REVISION_DATE)) {
        std::cerr << "[RvlCellAnimObject::RvlCellAnimObject] Invalid RvlCellAnim binary: revision date failed check!\n";
        return;
    }

    this->sheetIndex = BYTESWAP_16(header->sheetIndex);

    this->textureW = BYTESWAP_16(header->sheetW);
    this->textureH = BYTESWAP_16(header->sheetH);

    this->usePalette = header->usePalette != 0;

    unsigned readOffset{ sizeof(RvlCellAnimHeader) };

    // Arrangements
    const uint16_t arrangementCount = BYTESWAP_16(
        reinterpret_cast<const ArrangementsHeader*>(
            RvlCellAnimData + readOffset
        )->arrangementCount
    );
    readOffset += sizeof(ArrangementsHeader);

    this->arrangements.resize(arrangementCount);

    for (uint16_t i = 0; i < arrangementCount; i++) {
        const ArrangementRaw* arrangementRaw =
            reinterpret_cast<const ArrangementRaw*>(RvlCellAnimData + readOffset);
        readOffset += sizeof(ArrangementRaw);

        Arrangement& arrangement = this->arrangements[i];

        const uint16_t arrangementPartCount = BYTESWAP_16(arrangementRaw->partsCount);
        arrangement.parts.resize(arrangementPartCount);

        for (uint16_t j = 0; j < arrangementPartCount; j++) {
            const ArrangementPartRaw* arrangementPartRaw =
                reinterpret_cast<const ArrangementPartRaw*>(RvlCellAnimData + readOffset);
            readOffset += sizeof(ArrangementPartRaw);

            arrangement.parts[j] = ArrangementPart{
                .regionX = BYTESWAP_16(arrangementPartRaw->regionX),
                .regionY = BYTESWAP_16(arrangementPartRaw->regionY),
                .regionW = BYTESWAP_16(arrangementPartRaw->regionW),
                .regionH = BYTESWAP_16(arrangementPartRaw->regionH),

                .transform = arrangementPartRaw->transform.getByteswapped(),

                .flipX = arrangementPartRaw->flipX != 0,
                .flipY = arrangementPartRaw->flipY != 0,

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

    for (uint16_t i = 0; i < animationCount; i++) {
        const AnimationRaw* animationRaw = reinterpret_cast<const AnimationRaw*>(RvlCellAnimData + readOffset);
        readOffset += sizeof(AnimationRaw);

        Animation& animation = this->animations[i];

        const uint16_t keyCount = BYTESWAP_16(animationRaw->keyCount);
        animation.keys.resize(keyCount);

        for (uint16_t j = 0; j < keyCount; j++) {
            const AnimationKeyRaw* keyRaw = reinterpret_cast<const AnimationKeyRaw*>(RvlCellAnimData + readOffset);
            readOffset += sizeof(AnimationKeyRaw);

            animation.keys[j] = AnimationKey{
                .arrangementIndex = BYTESWAP_16(keyRaw->arrangementIndex),

                .holdFrames = BYTESWAP_16(keyRaw->holdFrames),

                .transform = keyRaw->transform.getByteswapped(),

                .opacity = keyRaw->opacity
            };
        }
    }

    this->expectEditorData =
        memcmp(RvlCellAnimData + dataSize - 8, EXPECT_DATA_FOOTER, 8) == 0;

    this->ok = true;
}

std::vector<unsigned char> RvlCellAnimObject::Reserialize() {
    // Allocate persistent size
    std::vector<unsigned char> result(
        sizeof(RvlCellAnimHeader) +
        sizeof(ArrangementsHeader) +
        sizeof(AnimationsHeader) +
        8 // EXPECT_DATA_FOOTER
    );

    RvlCellAnimHeader* header = reinterpret_cast<RvlCellAnimHeader*>(result.data());
    header->revisionDate = RCAD_REVISION_DATE;

    header->usePalette = this->usePalette ? 0x01000000 : 0x00000000;

    header->sheetIndex = BYTESWAP_16(this->sheetIndex);

    header->sheetW = BYTESWAP_16(this->textureW);
    header->sheetH = BYTESWAP_16(this->textureH);

    unsigned writeOffset{ sizeof(RvlCellAnimHeader) };

    *reinterpret_cast<ArrangementsHeader*>(result.data() + writeOffset) =
        ArrangementsHeader{ BYTESWAP_16(static_cast<uint16_t>(this->arrangements.size())) };
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

            *arrangementPartRaw = ArrangementPartRaw{
                .regionX = BYTESWAP_16(part.regionX),
                .regionY = BYTESWAP_16(part.regionY),
                .regionW = BYTESWAP_16(part.regionW),
                .regionH = BYTESWAP_16(part.regionH),

                .transform = part.transform.getByteswapped(),

                .flipX = part.flipX ? (uint8_t)0x01 : (uint8_t)0x00,
                .flipY = part.flipY ? (uint8_t)0x01 : (uint8_t)0x00,

                .opacity = part.opacity,
            };

            if (!part.editorVisible)
                arrangementPartRaw->opacity = 0;
        }
    }

    *reinterpret_cast<AnimationsHeader*>(result.data() + writeOffset) =
        AnimationsHeader{ BYTESWAP_16(static_cast<uint16_t>(this->animations.size())) };
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

            *animationKeyRaw = AnimationKeyRaw{
                .arrangementIndex = BYTESWAP_16(key.arrangementIndex),

                .holdFrames = BYTESWAP_16(key.holdFrames),

                .transform = key.transform.getByteswapped(),

                .opacity = key.opacity
            };
        }
    }

    snprintf(
        reinterpret_cast<char*>(result.data() + result.size() - 8), 8,
        "%.8s", EXPECT_DATA_FOOTER
    );

    return result;
}

std::shared_ptr<RvlCellAnimObject> readRvlCellAnimFile(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (UNLIKELY(!file.is_open())) {
        std::cerr << "[RvlCellAnim::readRvlCellAnimFile] Could not open file at path: " << filePath << '\n';
        return nullptr;
    }

    const std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    unsigned char* buffer = new unsigned char[fileSize];
    file.read(reinterpret_cast<char*>(buffer), fileSize);

    file.close();

    std::shared_ptr<RvlCellAnimObject> object =
        std::make_shared<RvlCellAnimObject>(RvlCellAnimObject(buffer, fileSize));

    delete[] buffer;

    return object;
}

} // namespace RvlCellAnim
