#include "CTPK.hpp"

#include <cstddef>

#include <algorithm>

#include "../Logging.hpp"

#include "../MainThreadTaskManager.hpp"

#include "CtrImageConvert.hpp"

#include "../CRC32.hpp"

#include "../stb/stb_image_resize2.h"

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
} __attribute__((packed));

struct CtpkFileHeader {
    uint32_t magic { CTPK_MAGIC }; // Compare to CTPK_MAGIC
    uint16_t formatVersion { CTPK_VERSION }; // Compare to CTPK_VERSION

    uint16_t textureCount;

    uint32_t dataSectionOffset; // Offset to the data section.
    uint32_t dataSectionSize; // Size of the data section.

    // Note: these entries are sorted by their hash.
    uint32_t lookupEntriesOffset; // Offset to the lookup entries (CtpkLookupEntry[textureCount]).

    // Note: these entries are pretty much useless for us, the only unique data given
    //       is the ETC1 compression method / quality.
    uint32_t infoEntriesOffset; // Offset to the info entries (CtpkInfoEntry[textureCount]).

    uint32_t _pad64[2] { 0x00000000, 0x00000000 };

    CtpkTextureEntry textureEntries[0];
} __attribute__((packed));

struct CtpkLookupEntry {
    uint32_t sourcePathHash; // CRC-32/ISO-HDLC hash of the source path.
    uint32_t textureEntryIndex;
} __attribute__((packed));

struct CtpkInfoEntry {
    uint8_t dataFormat; // Texture data format (CTPKImageFormat).

    uint8_t mipCount; // Mipmap count.

    uint8_t compressed; // Is texture data compressed (ETC1)?
    uint8_t compressionMethod;  // ETC1 compression method.
} __attribute__((packed));

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

        // CTPK textures are always clamped.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->data.data());

        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return textureId;
}

CTPKObject::CTPKObject(const unsigned char* ctpkData, const size_t dataSize) {
    if (dataSize < sizeof(CtpkFileHeader)) {
        Logging::err << "[CTPKObject::CTPKObject] Invalid CTPK binary: data size smaller than header size!" << std::endl;
        return;
    }

    const CtpkFileHeader* header = reinterpret_cast<const CtpkFileHeader*>(ctpkData);
    if (header->magic != CTPK_MAGIC) {
        Logging::err << "[CTPKObject::CTPKObject] Invalid CTPK binary: header magic is nonmatching!" << std::endl;
        return;
    }

    if (header->formatVersion != CTPK_VERSION) {
        Logging::err << "[CTPKObject::CTPKObject] Expected CTPK version " << CTPK_VERSION <<
            ", got version 0x" << header->formatVersion << " instead!";
        return;
    }

    mTextures.resize(header->textureCount);

    for (unsigned i = 0; i < header->textureCount; i++) {
        const CtpkTextureEntry* textureIn = header->textureEntries + i;
        CTPKTexture& textureOut = mTextures[i];

        if (textureIn->type != 2) {
            Logging::err << "[CTPKObject::CTPKObject] Unsupported texture (no. " << i+1 <<
                "): non-2D textures are not supported!" << std::endl;
            return;
        }

        textureOut.width = textureIn->width;
        textureOut.height = textureIn->height;

        textureOut.mipCount = textureIn->mipCount;

        textureOut.targetFormat = static_cast<CTPKImageFormat>(textureIn->dataFormat);

        textureOut.sourceTimestamp = textureIn->sourceTimestamp;
        if (textureIn->sourcePathOffset != 0) {
            textureOut.sourcePath = std::string(reinterpret_cast<const char*>(
                ctpkData + textureIn->sourcePathOffset
            ));
        }

        const unsigned expectedImageDataSize = CtrImageConvert::getImageByteSize(textureOut);
        if (textureIn->dataSize < expectedImageDataSize) {
            Logging::err << "[CTPKObject::CTPKObject] Invalid texture (no. " << i+1 <<
                "): image data size was less than expected (" << textureIn->dataSize << " < " <<
                expectedImageDataSize << ")" << std::endl;
            return;
        }

        const unsigned char* imageData = ctpkData + header->dataSectionOffset + textureIn->dataOffset;
        const unsigned char* imageDataEnd = imageData + textureIn->dataSize;

        if (getImageFormatCompressed(textureOut.targetFormat)) {
            // Copy targeted data.
            textureOut.cachedTargetData.assign(imageData, imageDataEnd);
        }

        // Copy RGBA data.
        textureOut.data.resize(textureOut.width * textureOut.height * 4);
        CtrImageConvert::toRGBA32(textureOut, imageData);
    }

    mInitialized = true;
}

std::vector<unsigned char> CTPKObject::Serialize() {
    std::vector<unsigned char> result;

    if (!mInitialized) {
        Logging::err << "[CTPKObject::Serialize] Unable to serialize: not initialized!" << std::endl;
        return result;
    }

    unsigned fullSize = sizeof(CtpkFileHeader);
    for (size_t i = 0; i < mTextures.size(); i++) {
        const auto& texture = mTextures[i];

        fullSize += sizeof(CtpkTextureEntry) + sizeof(CtpkLookupEntry) + sizeof(CtpkInfoEntry);

        fullSize += sizeof(uint32_t) * texture.mipCount;

        fullSize += ALIGN_UP_4(texture.sourcePath.size() + 1);
        fullSize += ALIGN_UP_32(CtrImageConvert::getImageByteSize(texture));
    }

    result.resize(fullSize);

    CtpkFileHeader* header = reinterpret_cast<CtpkFileHeader*>(result.data());
    *header = CtpkFileHeader {
        .textureCount = static_cast<uint16_t>(mTextures.size())
    };

    uint32_t* currentMipSize = reinterpret_cast<uint32_t*>(
        header->textureEntries + mTextures.size()
    );

    // Write texture entries.
    for (size_t i = 0; i < mTextures.size(); i++) {
        const auto& texture = mTextures[i];
        CtpkTextureEntry* texEntry = header->textureEntries + i;

        texEntry->dataSize = CtrImageConvert::getImageByteSize(texture);
        texEntry->dataFormat = static_cast<uint32_t>(texture.targetFormat);

        if (texture.width > 1024 || texture.height > 1024) {
            Logging::err << "[CTPKObject::Serialize] Texture no. " << i+1 << " exceeds the dimensions limit of 1024x1024; the" << std::endl;
            Logging::err << "                        texture will be scaled down to fit within the bounds." << std::endl;

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
            currentMipSize - reinterpret_cast<uint32_t*>(result.data())
        );

        unsigned currentWidth = texture.width;
        unsigned currentHeight = texture.height;
        for (unsigned i = 0; i < texture.mipCount; i++) {
            *(currentMipSize++) = CtrImageConvert::getImageByteSize(
                texture.targetFormat, currentWidth, currentHeight, 1
            );

            currentWidth = (currentWidth > 1) ? currentWidth / 2 : 1;
            currentHeight = (currentHeight > 1) ? currentHeight / 2 : 1;
        }

        texEntry->sourceTimestamp = texture.sourceTimestamp;
    }

    // Texture filenames follow the mipmap sizes.
    char* currentTexFilename = reinterpret_cast<char*>(currentMipSize);

    // Write texture filenames.
    for (size_t i = 0; i < mTextures.size(); i++) {
        const auto& texture = mTextures[i];
        CtpkTextureEntry* texEntry = header->textureEntries + i;

        texEntry->sourcePathOffset = static_cast<uint32_t>(
            currentTexFilename - reinterpret_cast<char*>(result.data())
        );

        strcpy(currentTexFilename, texture.sourcePath.c_str());
        currentTexFilename += ALIGN_UP_4(texture.sourcePath.size() + 1);
    }

    // Lookup entries follow the filenames.
    CtpkLookupEntry* lookupEntriesStart = reinterpret_cast<CtpkLookupEntry*>(
        currentTexFilename
    );
    CtpkLookupEntry* lookupEntriesEnd = lookupEntriesStart + mTextures.size();

    header->lookupEntriesOffset = static_cast<uint32_t>(
        reinterpret_cast<unsigned char*>(lookupEntriesStart) - result.data()
    );

    // Write lookup entries.
    for (size_t i = 0; i < mTextures.size(); i++) {
        CtpkLookupEntry* lookupEntry = lookupEntriesStart + i;

        lookupEntry->sourcePathHash = CRC32::compute(mTextures[i].sourcePath);
        lookupEntry->textureEntryIndex = i;
    }

    // Sort lookup entries by hash.
    std::sort(
        lookupEntriesStart, lookupEntriesEnd,
        [](const CtpkLookupEntry& a, const CtpkLookupEntry& b) {
            return a.sourcePathHash < b.sourcePathHash;
        }
    );

    // Info entries follow the lookup entries.
    CtpkInfoEntry* infoEntriesStart = reinterpret_cast<CtpkInfoEntry*>(
        lookupEntriesEnd
    );
    CtpkInfoEntry* infoEntriesEnd = infoEntriesStart + mTextures.size();

    header->infoEntriesOffset = static_cast<uint32_t>(
        reinterpret_cast<unsigned char*>(infoEntriesStart) - result.data()
    );

    // Write info entries.
    for (size_t i = 0; i < mTextures.size(); i++) {
        const auto& texture = mTextures[i];
        CtpkInfoEntry* infoEntry = infoEntriesStart + i;

        infoEntry->dataFormat = static_cast<uint8_t>(texture.targetFormat);
        infoEntry->mipCount = static_cast<uint8_t>(texture.mipCount);

        const bool isCompressed = getImageFormatCompressed(texture.targetFormat);

        infoEntry->compressed = isCompressed ? 1 : 0;
        infoEntry->compressionMethod = isCompressed ? 7 : 0;
    }

    // Data section follows the info entries.
    unsigned char* dataSectionStart = reinterpret_cast<unsigned char*>(
        infoEntriesEnd
    );
    header->dataSectionOffset = static_cast<uint32_t>(dataSectionStart - result.data());

    unsigned char* currentData = dataSectionStart;
    for (size_t i = 0; i < mTextures.size(); i++) {
        CtpkTextureEntry* texEntry = header->textureEntries + i;

        texEntry->dataOffset = static_cast<uint32_t>(currentData - dataSectionStart);

        const auto& srcTexture = mTextures[i];
        auto dstTexture = mTextures[i]; // Copy

        // If texture dimensions were clamped, scale down to new size.
        if (dstTexture.width != texEntry->width || dstTexture.height != texEntry->height) {
            dstTexture.width = texEntry->width;
            dstTexture.height = texEntry->height;

            dstTexture.data.resize(dstTexture.width * dstTexture.height * 4);

            stbir_resize_uint8_linear(
                srcTexture.data.data(), srcTexture.width, srcTexture.height,
                srcTexture.width * 4,
                dstTexture.data.data(), dstTexture.width, dstTexture.height,
                dstTexture.width * 4,
                STBIR_RGBA
            );
        }

        for (unsigned j = 0; j < dstTexture.mipCount; j++) {
            Logging::info <<
                "[CTPKObject::Serialize] Writing data for texture no. " << (i+1) << " (mip-level no. " <<
                j+1 << ") (" << dstTexture.width << 'x' << dstTexture.height << ", " <<
                getImageFormatName(dstTexture.targetFormat) << ").." << std::endl;

            CtrImageConvert::fromRGBA32(dstTexture, currentData);
            currentData += CtrImageConvert::getImageByteSize(
                dstTexture.targetFormat, dstTexture.width, dstTexture.height, 1
            );

            // Scale texture for next mip-level.
            if (j + 1 != dstTexture.mipCount) {
                dstTexture.width = (dstTexture.width > 1) ? dstTexture.width / 2 : 1;
                dstTexture.height = (dstTexture.height > 1) ? dstTexture.height / 2 : 1;

                // Not really necessary.
                // dstTexture.data.resize(dstTexture.width * dstTexture.height * 4);

                stbir_resize_uint8_linear(
                    srcTexture.data.data(), srcTexture.width, srcTexture.height,
                    srcTexture.width * 4,
                    dstTexture.data.data(), dstTexture.width, dstTexture.height,
                    dstTexture.width * 4,
                    STBIR_RGBA
                );
            }
        }
    }

    header->dataSectionSize = static_cast<uint32_t>(currentData - dataSectionStart);

    return result;
}

}
