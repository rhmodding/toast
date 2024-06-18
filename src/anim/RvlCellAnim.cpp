#include "RvlCellAnim.hpp"

#include <iostream>
#include <fstream>

#include <string>

#undef NDEBUG
#include <assert.h>

#include "../common.hpp"

#define HEADER_MAGIC 0xD8B43201

#pragma pack(push, 1) // Pack struct members tightly without padding

struct RvlCellAnimHeader {
    // Magic value (should always equal to 0x0132B4D8 [20100312] if valid)
    // Could be a format revision timestamp? (2010/03/12)
    uint32_t magic;

    // Unknown value
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

    // Value is uint8_t[4] but is later byte-swapped and casted to LE float
    uint8_t /* FLOAT */ scaleX[4], scaleY[4], angle[4];

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
    uint8_t offsetLeft, offsetRight;
    uint8_t offsetTop, offsetBottom;

    uint8_t /* FLOAT */ scaleX[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float
    uint8_t /* FLOAT */ scaleY[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float

    uint8_t /* FLOAT */ angle[4]; // Value is uint8_t[4] but is later byte-swapped and casted to LE float

    // Opacity, ranges from 0 - 255
    uint8_t opacity;

    // 24-byte padding
    uint8_t pad24[3];
};

#pragma pack(pop) // Reset packing alignment to default

namespace RvlCellAnim {
    RvlCellAnimObject::RvlCellAnimObject(const unsigned char* RvlCellAnimData, const size_t dataSize) {
        const RvlCellAnimHeader* header = reinterpret_cast<const RvlCellAnimHeader*>(RvlCellAnimData);

        if (header->magic != HEADER_MAGIC) {
            std::cerr << "[RvlCellAnimObject::RvlCellAnimObject] Invalid RvlCellAnim binary: header magic failed check!\n";
            return;
        }

        this->sheetIndex = BYTESWAP_16(header->sheetIndex);

        this->textureW = BYTESWAP_16(header->sheetW);
        this->textureH = BYTESWAP_16(header->sheetH);

        this->unknown.u32 = BYTESWAP_32(header->unknown_0);

        uint32_t readOffset{ sizeof(RvlCellAnimHeader) };

        // Arrangements
        uint16_t arrangementCount = BYTESWAP_16(header->arrangementCount);
        this->arrangements.resize(arrangementCount);

        for (uint16_t i = 0; i < arrangementCount; i++) {
            const ArrangementRaw* arrangementRaw = reinterpret_cast<const ArrangementRaw*>(RvlCellAnimData + readOffset);
            readOffset += sizeof(ArrangementRaw);

            Arrangement arrangement;

            uint16_t arrangementPartCount = BYTESWAP_16(arrangementRaw->partsCount);
            arrangement.parts.resize(arrangementPartCount);

            for (uint16_t j = 0; j < arrangementPartCount; j++) {
                const ArrangementPartRaw* arrangementPartRaw = reinterpret_cast<const ArrangementPartRaw*>(RvlCellAnimData + readOffset);
                readOffset += sizeof(ArrangementPartRaw);

                ArrangementPart arrangementPart;

                arrangementPart.regionX = BYTESWAP_16(arrangementPartRaw->regionX);
                arrangementPart.regionY = BYTESWAP_16(arrangementPartRaw->regionY);
                arrangementPart.regionW = BYTESWAP_16(arrangementPartRaw->regionW);
                arrangementPart.regionH = BYTESWAP_16(arrangementPartRaw->regionH);

                arrangementPart.positionX = BYTESWAP_16(arrangementPartRaw->positionX);
                arrangementPart.positionY = BYTESWAP_16(arrangementPartRaw->positionY);

                arrangementPart.scaleX = Common::readBigEndianFloat(&arrangementPartRaw->scaleX[0]);
                arrangementPart.scaleY = Common::readBigEndianFloat(&arrangementPartRaw->scaleY[0]);

                arrangementPart.angle = Common::readBigEndianFloat(&arrangementPartRaw->angle[0]);

                arrangementPart.flipX = arrangementPartRaw->flipX != 0;
                arrangementPart.flipY = arrangementPartRaw->flipY != 0;

                arrangementPart.opacity = arrangementPartRaw->opacity;

                arrangementPart.unknown.u32 = BYTESWAP_32(arrangementPartRaw->unknown_1);

                arrangement.parts[j] = arrangementPart;
            }

            this->arrangements[i] = arrangement;
        }

        uint16_t animationCount = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(RvlCellAnimData + readOffset));
        readOffset += sizeof(uint16_t);

        readOffset += sizeof(uint16_t); // Unknown value after animationCount

        this->animations.resize(animationCount);

        for (uint16_t i = 0; i < animationCount; i++) {
            const AnimationRaw* animationRaw = reinterpret_cast<const AnimationRaw*>(RvlCellAnimData + readOffset);
            readOffset += sizeof(AnimationRaw);

            Animation animation;

            uint16_t keyCount = BYTESWAP_16(animationRaw->keyCount);
            animation.keys.resize(keyCount);

            for (uint16_t j = 0; j < keyCount; j++) {
                const AnimationKeyRaw* keyRaw = reinterpret_cast<const AnimationKeyRaw*>(RvlCellAnimData + readOffset);
                readOffset += sizeof(AnimationKeyRaw);

                AnimationKey key;

                key.arrangementIndex = BYTESWAP_16(keyRaw->arrangementIndex);

                key.holdFrames = BYTESWAP_16(keyRaw->holdFrames);

                key.scaleX = Common::readBigEndianFloat(&keyRaw->scaleX[0]);
                key.scaleY = Common::readBigEndianFloat(&keyRaw->scaleY[0]);

                key.angle = Common::readBigEndianFloat(&keyRaw->angle[0]);

                key.opacity = keyRaw->opacity;

                key.offsetLeft = keyRaw->offsetLeft;
                key.offsetRight = keyRaw->offsetRight;
                key.offsetTop = keyRaw->offsetTop;
                key.offsetBottom = keyRaw->offsetBottom;

                animation.keys[j] = key;
            }

            this->animations[i] = animation;
        }
    
        this->ok = true;
    }

    std::vector<unsigned char> RvlCellAnimObject::Reserialize() {
        // Allocate header now
        std::vector<unsigned char> result(sizeof(RvlCellAnimHeader));

        RvlCellAnimHeader* header = reinterpret_cast<RvlCellAnimHeader*>(result.data());
        header->magic = HEADER_MAGIC;

        header->unknown_0 = 0x00000000;
        header->unknown_1 = 0x0000;
        header->unknown_2 = 0x0000;

        header->sheetIndex = BYTESWAP_16(this->sheetIndex);

        header->sheetW = BYTESWAP_16(this->textureW);
        header->sheetH = BYTESWAP_16(this->textureH);

        header->arrangementCount = BYTESWAP_16(static_cast<uint16_t>(this->arrangements.size()));

        size_t writeOffset{ sizeof(RvlCellAnimHeader) };

        for (const Arrangement& arrangement : this->arrangements) {
            result.resize(
                result.size() +
                sizeof(ArrangementRaw) +
                (sizeof(ArrangementPartRaw) * arrangement.parts.size())
            );

            ArrangementRaw* arrangementRaw = reinterpret_cast<ArrangementRaw*>(result.data() + writeOffset);
            writeOffset += sizeof(ArrangementRaw);

            arrangementRaw->partsCount = BYTESWAP_16(static_cast<uint16_t>(arrangement.parts.size()));
            arrangementRaw->unknown = 0x0000;

            for (const ArrangementPart& part : arrangement.parts) {
                ArrangementPartRaw* arrangementPartRaw =
                    reinterpret_cast<ArrangementPartRaw*>(result.data() + writeOffset);
                writeOffset += sizeof(ArrangementPartRaw);

                arrangementPartRaw->regionX = BYTESWAP_16(part.regionX);
                arrangementPartRaw->regionY = BYTESWAP_16(part.regionY);
                arrangementPartRaw->regionW = BYTESWAP_16(part.regionW);
                arrangementPartRaw->regionH = BYTESWAP_16(part.regionH);

                arrangementPartRaw->positionX = BYTESWAP_16(part.positionX);
                arrangementPartRaw->positionY = BYTESWAP_16(part.positionY);

                arrangementPartRaw->flipX = part.flipX ? 0x01 : 0x00;
                arrangementPartRaw->flipY = part.flipY ? 0x01 : 0x00;

                arrangementPartRaw->opacity = part.opacity;

                arrangementPartRaw->unknown_1 = BYTESWAP_32(part.unknown.u32);
                arrangementPartRaw->unknown_2 = 0x00;

                Common::writeBigEndianFloat(part.scaleX, arrangementPartRaw->scaleX);
                Common::writeBigEndianFloat(part.scaleY, arrangementPartRaw->scaleY);
                Common::writeBigEndianFloat(part.angle, arrangementPartRaw->angle);
            }
        }
    
        // Write animation count + unknown value
        {
            result.resize(
                result.size() +
                sizeof(uint16_t) + sizeof(uint16_t)
            );

            uint16_t* animationCount = reinterpret_cast<uint16_t*>(result.data() + writeOffset);
            uint16_t* unknownValue = reinterpret_cast<uint16_t*>(result.data() + writeOffset + sizeof(uint16_t));

            *animationCount = BYTESWAP_16(static_cast<uint16_t>(this->animations.size()));
            *unknownValue = 0x0000;

            writeOffset += sizeof(uint16_t) + sizeof(uint16_t);
        }

        for (const Animation& animation : this->animations) {
            result.resize(
                result.size() +
                sizeof(AnimationRaw) +
                (sizeof(AnimationKeyRaw) * animation.keys.size())
            );

            AnimationRaw* animationRaw = reinterpret_cast<AnimationRaw*>(result.data() + writeOffset);
            writeOffset += sizeof(AnimationRaw);

            animationRaw->keyCount = BYTESWAP_16(static_cast<uint16_t>(animation.keys.size()));
            animationRaw->unknown = 0x0000;

            for (const AnimationKey& key : animation.keys) {
                AnimationKeyRaw* animationKeyRaw =
                    reinterpret_cast<AnimationKeyRaw*>(result.data() + writeOffset);
                writeOffset += sizeof(AnimationKeyRaw);

                animationKeyRaw->arrangementIndex = BYTESWAP_16(key.arrangementIndex);

                animationKeyRaw->holdFrames = BYTESWAP_16(key.holdFrames);

                animationKeyRaw->opacity = key.opacity;

                Common::writeBigEndianFloat(key.scaleX, animationKeyRaw->scaleX);
                Common::writeBigEndianFloat(key.scaleY, animationKeyRaw->scaleY);
                Common::writeBigEndianFloat(key.angle, animationKeyRaw->angle);
                
                animationKeyRaw->offsetLeft = key.offsetLeft;
                animationKeyRaw->offsetRight = key.offsetRight;
                animationKeyRaw->offsetTop = key.offsetTop;
                animationKeyRaw->offsetBottom = key.offsetBottom;
            }
        }
    
        return result;
    }

    // Note: the object is dynamically allocated here. Make sure to delete it when you're done!
    RvlCellAnimObject* ObjectFromFile(const char* path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        //                                binary mode        seek to end

        if (!file.is_open()) {
            std::cerr << "[RvlCellAnim::ObjectFromFile] Could not open file at path: " << path << '\n';
            return nullptr;
        }

        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        char* buffer = new char[fileSize];

        assert(file.read(buffer, fileSize));

        file.close();

        RvlCellAnimObject* object = new RvlCellAnimObject(
            reinterpret_cast<unsigned char*>(buffer), fileSize
        );

        delete[] buffer;

        return object;
    }
}