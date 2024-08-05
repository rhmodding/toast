#include "RvlCellAnim.hpp"

#include <iostream>
#include <fstream>

#include "../common.hpp"

#define HEADER_MAGIC 0xD8B43201

#pragma pack(push, 1) // Pack struct members tightly without padding

struct RvlCellAnimHeader {
    // Magic value (should always equal to 0x0132B4D8 [20100312] if valid)
    // Could be a format revision timestamp? (2010/03/12)
    uint32_t magic;

    // Unknown value
    uint32_t unknown_0;

    // Index into cellanim.tpl for the associated sheet
    uint16_t sheetIndex;

    // Unknown value
    uint16_t unknown_1;

    uint16_t sheetW; // Sheet width in relation to UV regions
    uint16_t sheetH; // Sheet height in relation to UV regions

    // Amount of arrangements (commonly referred to as sprites)
    uint16_t arrangementCount;

    // Unknown value, possibly padding?
    uint16_t unknown_2;
};

struct ArrangementPartRaw {
    uint16_t regionX; // X position of UV region in spritesheet
    uint16_t regionY; // Y position of UV region in spritesheet
    uint16_t regionW; // Width of UV region in spritesheet
    uint16_t regionH; // Height of UV region in spritesheet

    // Unknown value
    uint32_t unknown_1;

    // Transform values
    int16_t positionX{ 0 }, positionY{ 0 };
    float scaleX, scaleY;
    float angle; // Angle in degrees

    uint8_t flipX; // Horizontally flipped (boolean)
    uint8_t flipY; // Vertically flipped (boolean)

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

    // Transform values
    int16_t positionX{ 0 }, positionY{ 0 };
    float scaleX, scaleY;
    float angle; // Angle in degrees

    uint8_t opacity;

    // 24-byte padding
    uint8_t pad24[3];
};

#pragma pack(pop) // Reset packing alignment to default

namespace RvlCellAnim {
    RvlCellAnimObject::RvlCellAnimObject(const unsigned char* RvlCellAnimData, const size_t dataSize) {
        const RvlCellAnimHeader* header = reinterpret_cast<const RvlCellAnimHeader*>(RvlCellAnimData);

        if (UNLIKELY(header->magic != HEADER_MAGIC)) {
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

                arrangementPart.scaleX = Common::byteswapFloat(arrangementPartRaw->scaleX);
                arrangementPart.scaleY = Common::byteswapFloat(arrangementPartRaw->scaleY);

                arrangementPart.angle = Common::byteswapFloat(arrangementPartRaw->angle);

                arrangementPart.flipX = arrangementPartRaw->flipX != 0;
                arrangementPart.flipY = arrangementPartRaw->flipY != 0;

                arrangementPart.opacity = arrangementPartRaw->opacity;

                arrangementPart.unknown.u32 = BYTESWAP_32(arrangementPartRaw->unknown_1);

                if (UNLIKELY(arrangementPart.unknown.u32 != 0)) {
                    std::cout <<
                        "[RvlCellAnimObject::RvlCellAnimObject] Arrangement Part has nonzero unknown value!:\n" <<
                        "   - Arrangement No.: " << i + 1 << "\n" <<
                        "   - Part No.: " << j + 1 << "\n" <<
                        "   - Value: " << arrangementPart.unknown.u32 << "\n";
                }

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

                key.positionX = BYTESWAP_16(keyRaw->positionX);
                key.positionY = BYTESWAP_16(keyRaw->positionY);

                key.scaleX = Common::byteswapFloat(keyRaw->scaleX);
                key.scaleY = Common::byteswapFloat(keyRaw->scaleY);

                key.angle = Common::byteswapFloat(keyRaw->angle);

                key.opacity = keyRaw->opacity;

                animation.keys[j] = key;
            }

            this->animations[i] = animation;
        }

        this->ok = true;
    }

    std::vector<unsigned char> RvlCellAnimObject::Reserialize() {
        // Allocate header now
        std::vector<unsigned char> result(sizeof(RvlCellAnimHeader) + 8);

        RvlCellAnimHeader* header = reinterpret_cast<RvlCellAnimHeader*>(result.data());
        header->magic = HEADER_MAGIC;

        header->unknown_0 = 0x00000000;
        header->unknown_1 = 0x0000;
        header->unknown_2 = 0x0000;

        header->sheetIndex = BYTESWAP_16(this->sheetIndex);

        header->sheetW = BYTESWAP_16(this->textureW);
        header->sheetH = BYTESWAP_16(this->textureH);

        header->arrangementCount = BYTESWAP_16(static_cast<uint16_t>(this->arrangements.size()));

        uint32_t writeOffset{ sizeof(RvlCellAnimHeader) };

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

                arrangementPartRaw->scaleX = Common::byteswapFloat(part.scaleX);
                arrangementPartRaw->scaleY = Common::byteswapFloat(part.scaleY);

                arrangementPartRaw->angle = Common::byteswapFloat(part.angle);

                arrangementPartRaw->flipX = part.flipX ? 0x01 : 0x00;
                arrangementPartRaw->flipY = part.flipY ? 0x01 : 0x00;

                arrangementPartRaw->opacity = part.opacity;

                arrangementPartRaw->unknown_1 = BYTESWAP_32(part.unknown.u32);
                arrangementPartRaw->unknown_2 = 0x00;
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

                animationKeyRaw->positionX = BYTESWAP_16(key.positionX);
                animationKeyRaw->positionY = BYTESWAP_16(key.positionY);

                animationKeyRaw->scaleX = Common::byteswapFloat(key.scaleX);
                animationKeyRaw->scaleY = Common::byteswapFloat(key.scaleY);

                animationKeyRaw->angle = Common::byteswapFloat(key.angle);

                animationKeyRaw->opacity = key.opacity;
            }
        }

        strcpy(reinterpret_cast<char*>(result.data() + result.size() - 8), "MWTOAST");

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
}
