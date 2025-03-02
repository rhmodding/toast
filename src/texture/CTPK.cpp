#include "CTPK.hpp"

#include <cstddef>

#include <algorithm>

#include "../MainThreadTaskManager.hpp"

#include "CtrImageConvert.hpp"

#include "../CRC32.hpp"

#include "../common.hpp"

constexpr uint32_t CTPK_MAGIC = IDENTIFIER_TO_U32('C','T','P','K');

constexpr uint16_t CTPK_VERSION = 1;

struct CtpkTextureEntry {
    uint32_t sourcePathOffset; // File offset to source texture path (null-terminated).

    uint32_t dataSize; // Size of texture data.
    uint32_t dataOffset; // Offset to texture data (relative to start of data section).
    uint32_t dataFormat; // Texture data format (CTPKImageFormat).

    uint16_t width; // Texture width
    uint16_t height; // Texture height.

    uint8_t mipCount; // Mipmap count.

    uint8_t type; // 0: Cube Map, 1: 1D, 2: 2D

    uint16_t cubeMapDirection;

    // In 4byte increments; multiply by 4 to get the true offset.
    uint32_t mipSizesOffset;

    uint32_t sourceTimestamp; // Unix timestamp of source texture.

    uint32_t mipSizes[0]; // Sizes for each mipmap level.
} __attribute((packed));

struct CtpkFileHeader {
    uint32_t magic { CTPK_MAGIC }; // Compare to CTPK_MAGIC
    uint16_t formatVersion { CTPK_VERSION }; // Compare to CTPK_VERSION

    uint16_t textureCount;

    uint32_t dataSectionOffset; // Offset to the data section.
    uint32_t dataSectionSize; // Size of the data section.

    // Note: these entries are sorted by their hash.
    uint32_t lookupEntriesOffset; // Offset to the lookup entries (CtpkLookupEntry[textureCount]).

    // Note: these entries is pretty much useless, the only unique data given
    //       here is the compression method.
    uint32_t infoEntriesOffset; // Offset to the info entries (CtpkInfoEntry[textureCount]).

    uint32_t _pad64[2] { 0x00000000, 0x00000000 };

    CtpkTextureEntry textureEntries[0];
} __attribute((packed));

struct CtpkLookupEntry {
    uint32_t sourcePathHash; // CRC-32/ISO-HDLC hash of the source path.
    uint32_t textureEntryIndex;
} __attribute((packed));

struct CtpkInfoEntry {
    uint8_t dataFormat; // Texture data format (CTPKImageFormat).

    uint8_t mipCount; // Mipmap count.

    uint8_t compressed; // Is texture data compressed (ETC1)?
    uint8_t compressionMethod;  // ETC1 compression method.
} __attribute((packed));

static void bilinearScale(
    unsigned srcWidth, unsigned srcHeight,
    unsigned dstWidth, unsigned dstHeight,
    const unsigned char* srcPixels, unsigned char* dstPixels
) {
    const float ratioX = static_cast<float>(srcWidth - 1) / dstWidth;
    const float ratioY = static_cast<float>(srcHeight - 1) / dstHeight;

    for (unsigned _y = 0; _y < dstHeight; _y++) {
        const float scaledY = ratioY * _y;
        const unsigned y    = static_cast<unsigned>(scaledY);
        const float diffY   = scaledY - y;

        const unsigned yOffset     = y * srcWidth * 4;
        const unsigned yOffsetNext = (y + 1) * srcWidth * 4;

        for (unsigned _x = 0; _x < dstWidth; _x++) {
            const float scaledX = ratioX * _x;
            const unsigned x    = static_cast<unsigned>(scaledX);
            const float diffX   = scaledX - x;

            const unsigned index     = yOffset + x * 4;
            const unsigned indexNext = yOffsetNext + x * 4;

            for (unsigned c = 0; c < 4; c++) {
                float topLeft     = srcPixels[index + c];
                float topRight    = srcPixels[index + 4 + c];
                float bottomLeft  = srcPixels[indexNext + c];
                float bottomRight = srcPixels[indexNext + 4 + c];

                float channel = topLeft * (1 - diffX) * (1 - diffY) +
                                topRight * diffX * (1 - diffY) +
                                bottomLeft * (1 - diffX) * diffY +
                                bottomRight * diffX * diffY;

                dstPixels[(_y * dstWidth + _x) * 4 + c] = static_cast<unsigned char>(channel);
            }
        }
    }
}

namespace CTPK {

void CTPKTexture::rotateCCW() {
    // Copy
    std::vector<unsigned char> temp = this->data;

    for (unsigned y = 0; y < this->height; ++y) {
        for (unsigned x = 0; x < this->width; ++x) {
            unsigned newX = y;
            unsigned newY = (this->width - 1) - x;

            unsigned oldIndex = (y * this->width + x) * 4;
            unsigned newIndex = (newY * this->height + newX) * 4;

            this->data[newIndex]     = temp[oldIndex];
            this->data[newIndex + 1] = temp[oldIndex + 1];
            this->data[newIndex + 2] = temp[oldIndex + 2];
            this->data[newIndex + 3] = temp[oldIndex + 3];
        }
    }

    unsigned width = this->width;
    this->width = this->height;
    this->height = width;
}

void CTPKTexture::rotateCW() {
    // Copy
    std::vector<unsigned char> temp = this->data;

    for (unsigned y = 0; y < this->height; ++y) {
        for (unsigned x = 0; x < this->width; ++x) {
            unsigned newX = (this->height - 1) - y;
            unsigned newY = x;

            unsigned oldIndex = (y * this->width + x) * 4;
            unsigned newIndex = (newY * this->height + newX) * 4;

            this->data[newIndex]     = temp[oldIndex];
            this->data[newIndex + 1] = temp[oldIndex + 1];
            this->data[newIndex + 2] = temp[oldIndex + 2];
            this->data[newIndex + 3] = temp[oldIndex + 3];
        }
    }

    unsigned width = this->width;
    this->width = this->height;
    this->height = width;
}

GLuint CTPKTexture::createGPUTexture() const {
    GLuint textureId { 0 };

    MainThreadTaskManager::getInstance().QueueTask([this, &textureId]() {
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->data.data());

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return textureId;
}

CTPKObject::CTPKObject(const unsigned char* ctpkData, const size_t dataSize) {
    if (dataSize < sizeof(CtpkFileHeader)) {
        std::cerr << "[CTPKObject::CTPKObject] Invalid CTPK binary: data size smaller than header size!\n";
        return;
    }

    const CtpkFileHeader* header = reinterpret_cast<const CtpkFileHeader*>(ctpkData);
    if (header->magic != CTPK_MAGIC) {
        std::cerr << "[CTPKObject::CTPKObject] Invalid CTPK binary: header magic is nonmatching!\n";
        return;
    }

    if (header->formatVersion != CTPK_VERSION) {
        std::cerr << "[CTPKObject::CTPKObject] Expected CTPK version " << CTPK_VERSION <<
            ", got version 0x" << header->formatVersion << " instead!";
        return;
    }

    this->textures.resize(header->textureCount);

    for (unsigned i = 0; i < header->textureCount; i++) {
        const CtpkTextureEntry* textureIn = header->textureEntries + i;
        CTPKTexture& textureOut = this->textures[i];

        if (textureIn->type != 2) {
            std::cerr << "[CTPKObject::CTPKObject] Unsupported texture (no. " << i+1 <<
                "): non-2D textures are not supported!\n";
            return;
        }

        textureOut.width = textureIn->width;
        textureOut.height = textureIn->height;

        textureOut.mipCount = textureIn->mipCount;

        textureOut.format = static_cast<CTPKImageFormat>(textureIn->dataFormat);

        textureOut.sourceTimestamp = textureIn->sourceTimestamp;
        if (textureIn->sourcePathOffset != 0) {
            textureOut.sourcePath = std::string(reinterpret_cast<const char*>(
                ctpkData + textureIn->sourcePathOffset
            ));
        }

        const unsigned char* imageData = ctpkData + header->dataSectionOffset + textureIn->dataOffset;

        textureOut.data.resize(textureOut.width * textureOut.height * 4);
        CtrImageConvert::toRGBA32(textureOut, imageData);
    }

    this->ok = true;
}

std::vector<unsigned char> CTPKObject::Serialize() {
    unsigned fullSize = sizeof(CtpkFileHeader) + ((
        sizeof(CtpkTextureEntry) +
        sizeof(CtpkLookupEntry) +
        sizeof(CtpkInfoEntry)
    ) * this->textures.size());

    for (unsigned i = 0; i < this->textures.size(); i++) {
        const auto& texture = this->textures[i];

        fullSize += sizeof(uint32_t) * texture.mipCount;

        fullSize += ALIGN_UP_4(texture.sourcePath.size() + 1);
        fullSize += ALIGN_UP_32(CtrImageConvert::getImageByteSize(texture));
    }

    std::vector<unsigned char> result(fullSize);

    CtpkFileHeader* header = reinterpret_cast<CtpkFileHeader*>(result.data());
    *header = CtpkFileHeader {
        .textureCount = static_cast<uint16_t>(this->textures.size())
    };

    uint32_t* currentMipSize = reinterpret_cast<uint32_t*>(
        header->textureEntries + this->textures.size()
    );

    // Write texture entries.
    for (unsigned i = 0; i < this->textures.size(); i++) {
        const auto& texture = this->textures[i];
        CtpkTextureEntry* texEntry = header->textureEntries + i;
        
        texEntry->dataSize = CtrImageConvert::getImageByteSize(texture);
        texEntry->dataFormat = static_cast<uint32_t>(texture.format);

        if (texture.width > 1024 || texture.height > 1024) {
            std::cerr <<
                "[CTPKObject::Serialize] Texture no. " << i+1 << " exceeds the dimensions limit of 1024x1024; the\n"
                "                        texture will be scaled down to fit within the bounds.";
            
            float scale = std::min(
                1024.f / texture.width,
                1024.f / texture.height
            );

            texEntry->width = static_cast<uint16_t>(texture.width * scale);
            texEntry->height = static_cast<uint16_t>(texture.height * scale);
        }
        else {
            texEntry->width = static_cast<uint16_t>(texture.width);
            texEntry->height = static_cast<uint16_t>(texture.height);
        }

        texEntry->mipCount = static_cast<uint8_t>(texture.mipCount);

        texEntry->type = 2; // 2D texture.
        texEntry->cubeMapDirection = 0;

        // mipSizesOffset is in 4byte increments.
        texEntry->mipSizesOffset = static_cast<uint32_t>(
            currentMipSize - (uint32_t*)result.data()
        );

        unsigned currentWidth = texture.width;
        unsigned currentHeight = texture.height;
        for (unsigned i = 0; i < texture.mipCount; i++) {
            *(currentMipSize++) = CtrImageConvert::getImageByteSize(
                texture.format, currentWidth, currentHeight, 1
            );

            currentWidth = (currentWidth > 1) ? currentWidth / 2 : 1;
            currentHeight = (currentHeight > 1) ? currentHeight / 2 : 1;
        }

        texEntry->sourceTimestamp = texture.sourceTimestamp;
    }

    // Texture filenames follow the mipmap sizes.
    char* currentTexFilename = reinterpret_cast<char*>(currentMipSize);

    // Write texture filenames.
    for (unsigned i = 0; i < this->textures.size(); i++) {
        const auto& texture = this->textures[i];
        CtpkTextureEntry* texEntry = header->textureEntries + i;

        texEntry->sourcePathOffset = static_cast<uint32_t>(
            currentTexFilename - (char*)result.data()
        );

        strcpy(currentTexFilename, texture.sourcePath.c_str());
        currentTexFilename += ALIGN_UP_4(texture.sourcePath.size() + 1);
    }

    // Lookup entries follow the filenames.
    CtpkLookupEntry* lookupEntriesStart = reinterpret_cast<CtpkLookupEntry*>(
        currentTexFilename
    );
    CtpkLookupEntry* lookupEntriesEnd = lookupEntriesStart + this->textures.size();

    header->lookupEntriesOffset = static_cast<uint32_t>(
        (unsigned char*)lookupEntriesStart - result.data()
    );

    // Write lookup entries.
    for (unsigned i = 0; i < this->textures.size(); i++) {
        CtpkLookupEntry* lookupEntry = lookupEntriesStart + i;

        lookupEntry->sourcePathHash = CRC32::compute(this->textures[i].sourcePath);
        lookupEntry->textureEntryIndex = i;
    }

    // Sort lookup entries by hash.
    std::sort(lookupEntriesStart, lookupEntriesEnd,
    [](const CtpkLookupEntry& a, const CtpkLookupEntry& b) {
        return a.sourcePathHash < b.sourcePathHash;
    });

    // Info entries follow the lookup entries.
    CtpkInfoEntry* infoEntriesStart = reinterpret_cast<CtpkInfoEntry*>(
        lookupEntriesEnd
    );
    CtpkInfoEntry* infoEntriesEnd = infoEntriesStart + this->textures.size();

    header->infoEntriesOffset = static_cast<uint32_t>(
        (unsigned char*)infoEntriesStart - result.data()
    );

    // Write info entries.
    for (unsigned i = 0; i < this->textures.size(); i++) {
        const auto& texture = this->textures[i];
        CtpkInfoEntry* infoEntry = infoEntriesStart + i;

        infoEntry->dataFormat = static_cast<uint8_t>(texture.format);
        infoEntry->mipCount = static_cast<uint8_t>(texture.mipCount);

        const bool isCompressed =
            texture.format == CTPK_IMAGE_FORMAT_ETC1 ||
            texture.format == CTPK_IMAGE_FORMAT_ETC1A4;

        infoEntry->compressed = isCompressed ? 1 : 0;
        infoEntry->compressionMethod = isCompressed ? 7 : 0;
    }

    // Data section follows the info entries.
    unsigned char* dataSectionStart = reinterpret_cast<unsigned char*>(
        infoEntriesEnd
    );
    header->dataSectionOffset = static_cast<uint32_t>(dataSectionStart - result.data());

    unsigned char* currentData = dataSectionStart;
    for (unsigned i = 0; i < this->textures.size(); i++) {
        CtpkTextureEntry* texEntry = header->textureEntries + i;

        texEntry->dataOffset = static_cast<uint32_t>(currentData - dataSectionStart);

        const auto& srcTexture = this->textures[i];
        auto dstTexture = this->textures[i]; // Copy

        // If texture dimensions were clamped, scale down to new size.
        if (dstTexture.width != texEntry->width || dstTexture.height != texEntry->height) {
            bilinearScale(
                dstTexture.width, dstTexture.height,
                texEntry->width, texEntry->height,
                srcTexture.data.data(), dstTexture.data.data()
            );

            dstTexture.width = texEntry->width;
            dstTexture.height = texEntry->height;
        }

        for (unsigned i = 0; i < dstTexture.mipCount; i++) {
            CtrImageConvert::fromRGBA32(dstTexture, currentData);
            currentData += CtrImageConvert::getImageByteSize(
                dstTexture.format, dstTexture.width, dstTexture.height, 1
            );

            unsigned newWidth = (dstTexture.width > 1) ? dstTexture.width / 2 : 1;
            unsigned newHeight = (dstTexture.height > 1) ? dstTexture.height / 2 : 1;

            if (i + 1 != dstTexture.mipCount)
                bilinearScale(
                    dstTexture.width, dstTexture.height,
                    newWidth, newHeight,
                    srcTexture.data.data(), dstTexture.data.data()
                );
            
            dstTexture.width = newWidth;
            dstTexture.height = newHeight;
        }
    }

    header->dataSectionSize = static_cast<uint32_t>(currentData - dataSectionStart);

    return result;
}

}
