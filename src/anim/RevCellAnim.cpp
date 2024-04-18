#include "RevCellAnim.hpp"

#include <iostream>
#include <fstream>

#include <string>

#include <assert.h>

#include "../common.hpp"

#pragma pack(push, 1) // Pack struct members tightly without padding

struct RevCellAnimHeader {
    // Magic value (should always equal to 0x0132B4D8 [20100312] if valid)
    uint32_t magic;

    // Unknown value. Usually 0x04000400 [67109888]
    uint32_t unknown_0;

    // Sheet index. The sheet will be located in cellanim.tpl under "cellanim_{sheetIndex}"
    uint16_t sheetIndex;

    // Unknown value
    uint16_t unknown_1;
    
    uint16_t sheetW; // Sheet width
    uint16_t sheetH; // Sheet height

    // Amount of arrangements (commonly referred to as sprites)
    uint16_t arrangementCount;

    // Unknown value, possibly padding?
    uint16_t unknown_2;
};

struct ArrangementPartRaw {
    uint16_t regionX; // X position of part in spritesheet
    uint16_t regionY; // Y position of part in spritesheet
    uint16_t regionW; // Width of part in spritesheet
    uint16_t regionH; // Height of part in spritesheet

    // Unknown value
    uint32_t unknown_1;

    int16_t positionX; // X position of rendered part (originates from top left)
    int16_t positionY; // Y position of rendered part (originates from top left)

    uint8_t /* FLOAT */ scaleX[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float
    uint8_t /* FLOAT */ scaleY[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float

    uint8_t /* FLOAT */ angle[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

    // 0 - 255
    uint8_t opacity;

    // Unknown value, possibly padding?
    uint8_t unknown_2;
};

struct ArrangementRaw {
    // Amout of parts in the arrangement (commonly referred to as a sprite)
    uint16_t partsCount;

    // Unknown value, possibly padding?
    uint16_t unknown;
};

struct AnimationRaw {
    // Amount of keys the animation has
    uint16_t keyCount;

    // Unknown value, possibly padding?
    uint16_t unknown;
};

struct AnimationKeyRaw {
    // Index of arrangement (commonly referred to as sprites)
    uint16_t arrangementIndex;

    // Amount of frames the key is held for
    uint16_t holdFrames;

    // Unknown value
    uint32_t unknown;

    uint8_t /* FLOAT */ scaleX[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float
    uint8_t /* FLOAT */ scaleY[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float

    uint8_t /* FLOAT */ angle[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float

    // Opacity, ranges from 0 - 255
    uint8_t opacity;

    // 24-byte padding
    uint8_t pad24[3];
};

#pragma pack(pop) // Reset packing alignment to default

// Byte-swap and cast uint8_t[4] to LE float
float readBigEndianFloat(const uint8_t* bytes) {
    uint32_t intValue = (static_cast<uint32_t>(bytes[0]) << 24) |
                        (static_cast<uint32_t>(bytes[1]) << 16) |
                        (static_cast<uint32_t>(bytes[2]) << 8) |
                        static_cast<uint32_t>(bytes[3]);

    float result;
    std::memcpy(&result, &intValue, sizeof(float));
    
    return result;
}

namespace RevCellAnim {
    RevCellAnimObject::RevCellAnimObject(const char* revCellAnimData, const size_t dataSize) {
        const RevCellAnimHeader* header = reinterpret_cast<const RevCellAnimHeader*>(revCellAnimData);

        if (BYTESWAP_32(header->magic) != 0x0132B4D8) {
            std::cerr << "[RevCellAnimObject::RevCellAnimObject] Invalid RevCellAnim binary: header magic failed check!\n";
            return;
        }

        this->sheetIndex = BYTESWAP_16(header->sheetIndex);

        size_t readOffset = sizeof(RevCellAnimHeader);

        // Arrangements
        uint16_t arrangementCount = BYTESWAP_16(header->arrangementCount);
        this->arrangements.resize(arrangementCount);

        for (uint16_t i = 0; i < arrangementCount; i++) {
            const ArrangementRaw* arrangementRaw = reinterpret_cast<const ArrangementRaw*>(revCellAnimData + readOffset);
            readOffset += sizeof(ArrangementRaw);

            Arrangement arrangement;

            uint16_t arrangementPartCount = BYTESWAP_16(arrangementRaw->partsCount);
            arrangement.parts.resize(arrangementPartCount);

            for (uint16_t j = 0; j < arrangementPartCount; j++) {
                const ArrangementPartRaw* arrangementPartRaw = reinterpret_cast<const ArrangementPartRaw*>(revCellAnimData + readOffset);
                readOffset += sizeof(ArrangementPartRaw);

                ArrangementPart arrangementPart;

                arrangementPart.regionX = BYTESWAP_16(arrangementPartRaw->regionX);
                arrangementPart.regionY = BYTESWAP_16(arrangementPartRaw->regionY);
                arrangementPart.regionW = BYTESWAP_16(arrangementPartRaw->regionW);
                arrangementPart.regionH = BYTESWAP_16(arrangementPartRaw->regionH);

                arrangementPart.positionX = BYTESWAP_16(arrangementPartRaw->positionX);
                arrangementPart.positionY = BYTESWAP_16(arrangementPartRaw->positionY);

                arrangementPart.scaleX = readBigEndianFloat(&arrangementPartRaw->scaleX[0]);
                arrangementPart.scaleY = readBigEndianFloat(&arrangementPartRaw->scaleY[0]);

                arrangementPart.angle = readBigEndianFloat(&arrangementPartRaw->angle[0]);

                arrangementPart.flipX = arrangementPartRaw->flipX != 0;
                arrangementPart.flipY = arrangementPartRaw->flipY != 0;

                arrangementPart.opacity = arrangementPartRaw->opacity;

                arrangement.parts[j] = arrangementPart;
            }

            this->arrangements[i] = arrangement;
        }

        uint16_t animationCount = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(revCellAnimData + readOffset));
        readOffset += sizeof(uint16_t);

        readOffset += sizeof(uint16_t); // Unknown value after animationCount

        this->animations.resize(animationCount);

        for (uint16_t i = 0; i < animationCount; i++) {
            const AnimationRaw* animationRaw = reinterpret_cast<const AnimationRaw*>(revCellAnimData + readOffset);
            readOffset += sizeof(AnimationRaw);

            Animation animation;

            uint16_t keyCount = BYTESWAP_16(animationRaw->keyCount);
            animation.keys.resize(keyCount);

            for (uint16_t j = 0; j < keyCount; j++) {
                const AnimationKeyRaw* keyRaw = reinterpret_cast<const AnimationKeyRaw*>(revCellAnimData + readOffset);
                readOffset += sizeof(AnimationKeyRaw);

                AnimationKey key;

                key.arrangementIndex = BYTESWAP_16(keyRaw->arrangementIndex);

                key.holdFrames = BYTESWAP_16(keyRaw->holdFrames);

                key.scaleX = readBigEndianFloat(&keyRaw->scaleX[0]);
                key.scaleY = readBigEndianFloat(&keyRaw->scaleY[0]);

                key.angle = readBigEndianFloat(&keyRaw->angle[0]);

                key.opacity = keyRaw->opacity;

                animation.keys[j] = key;
            }

            this->animations[i] = animation;
        }
    }

    // Note: the object is dynamically allocated here. Make sure to delete it when you're done!
    RevCellAnimObject* ObjectFromFile(const char* path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        //                                binary mode        seek to end

        assert(file.is_open());

        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        char* buffer = new char[fileSize];

        assert(file.read(buffer, fileSize));

        file.close();

        RevCellAnimObject* object = new RevCellAnimObject(buffer, fileSize);

        delete[] buffer;

        return object;
    }
}