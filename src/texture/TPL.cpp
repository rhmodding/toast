#include "TPL.hpp"

#include <iostream>
#include <fstream>

#include <unordered_set>

#include "../MainThreadTaskManager.hpp"

#include "RvlImageConvert.hpp"

#include "PaletteUtils.hpp"

#include "../common.hpp"

// Pre-byteswapped.
constexpr uint32_t TPL_VERSION_NUMBER = 0x30AF2000;

struct TPLClutHeader {
    uint16_t numEntries;

    // Boolean value, used in official SDK to keep track of TPL state.
    uint8_t _unpacked { 0x00 };

    uint8_t _pad8 { 0x00 };

    uint32_t format; // TPLClutFormat (big endian)

    uint32_t dataOffset;
} __attribute__((packed));

struct TPLPalette {
    // 2142000 in big-endian. This is a date (Feb 14 2000).
    uint32_t versionNumber { TPL_VERSION_NUMBER };
    
    // Amount of descriptors in the descriptor table.
    uint32_t descriptorCount;

    // Offset to the start of the descriptor table.
    uint32_t descriptorsOffset;
} __attribute__((packed));

struct TPLDescriptor {
    // Offset to the start of the associated texture header (TPLHeader).
    uint32_t textureHeaderOffset;

    // Offset to the begin of the associated CLUT header (TPLClutHeader).
    // (Can be NULL.)
    uint32_t CLUTHeaderOffset;
} __attribute__((packed));

struct TPLHeader {
    uint16_t height; // Texture height
    uint16_t width; // Texture width

    uint32_t format; // TPLImageFormat (big endian)

    uint32_t dataOffset;

    uint32_t wrapS; // TPLWrapMode (big endian)
    uint32_t wrapT; // TPLWrapMode (big endian)

    uint32_t minFilter; // TPLTexFilter (big endian)
    uint32_t magFilter; // TPLTexFilter (big endian)

    float LODBias;

    uint8_t edgeLODEnable; // Boolean value

    uint8_t minLOD;
    uint8_t maxLOD;

    // Boolean value, used in official SDK to keep track of status
    uint8_t _unpacked { 0x00 };
} __attribute__((packed));

namespace TPL {

GLuint TPLTexture::createGPUTexture() const {
    GLuint textureId { 0 };

    GLint minFilter { GL_LINEAR };
    GLint magFilter { GL_LINEAR };

    switch (this->minFilter) {
    case TPL::TPL_TEX_FILTER_NEAR:
        minFilter = GL_NEAREST;
        break;
    case TPL::TPL_TEX_FILTER_LINEAR:
        break;
    case TPL::TPL_TEX_FILTER_NEAR_MIP_NEAR:
        minFilter = GL_NEAREST_MIPMAP_NEAREST;
        break;
    case TPL::TPL_TEX_FILTER_LIN_MIP_NEAR:
        minFilter = GL_LINEAR_MIPMAP_NEAREST;
        break;
    case TPL::TPL_TEX_FILTER_NEAR_MIP_LIN:
        minFilter = GL_NEAREST_MIPMAP_LINEAR;
        break;
    case TPL::TPL_TEX_FILTER_LIN_MIP_LIN:
        minFilter = GL_LINEAR_MIPMAP_LINEAR;
        break;
    default:
        break;
    }

    switch (this->magFilter) {
    case TPL::TPL_TEX_FILTER_NEAR:
        magFilter = GL_NEAREST;
        break;
    case TPL::TPL_TEX_FILTER_LINEAR:
        break;
    default:
        break;
    }

    MainThreadTaskManager::getInstance().QueueTask([this, &textureId, &minFilter, &magFilter]() {
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
            this->wrapS == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
        );
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
            this->wrapT == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->data.data());

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return textureId;
}

TPLObject::TPLObject(const unsigned char* tplData, const size_t dataSize) {
    if (dataSize < sizeof(TPLPalette)) {
        std::cerr << "[TPLObject::TPLObject] Invalid TPL binary: data size smaller than palette size!\n";
        return;
    }

    const TPLPalette* palette = reinterpret_cast<const TPLPalette*>(tplData);
    if (palette->versionNumber != TPL_VERSION_NUMBER) {
        std::cerr << "[TPLObject::TPLObject] Invalid TPL binary: invalid version number!\n";
        return;
    }

    uint32_t descriptorCount = BYTESWAP_32(palette->descriptorCount);
    const TPLDescriptor* descriptors = reinterpret_cast<const TPLDescriptor*>(
        tplData + BYTESWAP_32(palette->descriptorsOffset)
    );

    this->textures.resize(descriptorCount);

    for (uint32_t i = 0; i < descriptorCount; i++) {
        const TPLDescriptor* descriptor = descriptors + i;

        if (descriptor->textureHeaderOffset == 0) {
            std::cerr << "[TPLObject::TPLObject] Texture no. " << i+1 << " could not be read (texture header offset is zero)!\n";
            return;
        }

        const TPLHeader* header = reinterpret_cast<const TPLHeader*>(tplData + BYTESWAP_32(descriptor->textureHeaderOffset));

        TPLTexture& textureData = this->textures[i];

        if (descriptor->CLUTHeaderOffset != 0) {
            const TPLClutHeader* clutHeader = reinterpret_cast<const TPLClutHeader*>(tplData + BYTESWAP_32(descriptor->CLUTHeaderOffset));

            PaletteUtils::CLUTtoRGBA32Palette(
                textureData.palette,

                tplData + BYTESWAP_32(clutHeader->dataOffset),
                BYTESWAP_16(clutHeader->numEntries),

                static_cast<TPL::TPLClutFormat>(BYTESWAP_32(clutHeader->format))
            );
        }

        textureData.width = BYTESWAP_16(header->width);
        textureData.height = BYTESWAP_16(header->height);

        textureData.format = static_cast<TPLImageFormat>(BYTESWAP_32(header->format));

        textureData.mipCount = header->maxLOD - header->minLOD + 1;

        textureData.wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(header->wrapS));
        textureData.wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(header->wrapT));

        textureData.minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header->minFilter));
        textureData.magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header->magFilter));

        // Copy image data
        const unsigned char* imageData = tplData + BYTESWAP_32(header->dataOffset);

        textureData.data.resize(textureData.width * textureData.height * 4);
        RvlImageConvert::toRGBA32(textureData, imageData);
    }

    this->ok = true;
}

std::vector<unsigned char> TPLObject::Serialize() {
    std::vector<unsigned char> result;

    const unsigned textureCount = this->textures.size();

    // Precompute size required & texture indexes for color palettes
    unsigned paletteEntriesSize { 0 };

    struct PaletteTexEntry {
        unsigned texIndex;
        std::set<uint32_t> palette;
    };
    std::vector<PaletteTexEntry> paletteTextures;
    paletteTextures.reserve(textureCount);

    for (unsigned i = 0; i < textureCount; i++) {
        const auto& texture = this->textures[i];

        if (
            texture.format == TPL_IMAGE_FORMAT_C4 ||
            texture.format == TPL_IMAGE_FORMAT_C8 ||
            texture.format == TPL_IMAGE_FORMAT_C14X2
        ) {
            paletteTextures.push_back(PaletteTexEntry {
                .texIndex = i,
                .palette = PaletteUtils::generatePalette(
                    texture.data.data(), texture.width * texture.height
                )
            });

            // Convieniently, every palette format's pixel is 16-bit
            paletteEntriesSize +=
                ALIGN_UP_16(paletteTextures.back().palette.size()) * 2;
        }
    }

    unsigned headerSectionStart = (
        ALIGN_UP_32(
            sizeof(TPLPalette) +
            (sizeof(TPLDescriptor) * textureCount) +
            (sizeof(TPLClutHeader) * paletteTextures.size())
        ) + paletteEntriesSize
    );

    unsigned dataSectionStart = headerSectionStart + (sizeof(TPLHeader) * textureCount);
    
    unsigned fullSize = dataSectionStart;

    // Precompute size of data section
    for (unsigned i = 0; i < textureCount; i++) {
        const unsigned imageSize = RvlImageConvert::getImageByteSize(this->textures[i]);

        fullSize = ALIGN_UP_32(fullSize);
        fullSize += imageSize;
    }

    result.resize(fullSize);

    TPLPalette* palette = reinterpret_cast<TPLPalette*>(result.data());
    *palette = TPLPalette {
        .descriptorCount = BYTESWAP_32(textureCount),
        .descriptorsOffset = BYTESWAP_32(sizeof(TPLPalette))
    };
    palette->descriptorCount = BYTESWAP_32(textureCount);
    palette->descriptorsOffset = BYTESWAP_32(sizeof(TPLPalette));

    unsigned nextClutOffset { 0 };
    if (!paletteTextures.empty())
        nextClutOffset = ALIGN_UP_32(
            sizeof(TPLPalette) +
            (sizeof(TPLDescriptor) * textureCount) +
            (sizeof(TPLClutHeader) * paletteTextures.size())
        );

    TPLHeader* headers = reinterpret_cast<TPLHeader*>(result.data() + headerSectionStart);
    TPLDescriptor* descriptors = reinterpret_cast<TPLDescriptor*>(result.data() + sizeof(TPLPalette));

    // Texture Descriptors
    for (unsigned textureIndex = 0; textureIndex < textureCount; textureIndex++) {
        TPLDescriptor* descriptor = descriptors + textureIndex;

        descriptor->textureHeaderOffset = BYTESWAP_32(static_cast<uint32_t>(
            headerSectionStart +
            (sizeof(TPLHeader) * textureIndex)
        ));

        auto it = std::find_if(
            paletteTextures.begin(), paletteTextures.end(),
            [textureIndex](const PaletteTexEntry& entry) {
                return entry.texIndex == textureIndex;
            }
        );
        if (it != paletteTextures.end()) {
            unsigned clutIndex = std::distance(paletteTextures.begin(), it);
            descriptor->CLUTHeaderOffset = BYTESWAP_32(static_cast<uint32_t>(
                sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * textureCount) +
                (sizeof(TPLClutHeader) * clutIndex)
            ));
        }
        else
            descriptor->CLUTHeaderOffset = 0x00000000;
    }

    TPLClutHeader* clutHeaders = reinterpret_cast<TPLClutHeader*>(
        result.data() +
        sizeof(TPLPalette) +
        (sizeof(TPLDescriptor) * textureCount)
    );

    // Palette Descriptors & Data
    for (unsigned clutIndex = 0; clutIndex < paletteTextures.size(); clutIndex++) {
        TPLClutHeader* clutHeader = clutHeaders + clutIndex;

        // RGB5A3 is the only CLUT format with support for transparency & full color at
        // the same time.
        clutHeader->format = BYTESWAP_32(TPL::TPL_CLUT_FORMAT_RGB5A3);
        clutHeader->dataOffset = BYTESWAP_32(nextClutOffset);

        unsigned colorCount = ALIGN_UP_16(paletteTextures[clutIndex].palette.size());
        clutHeader->numEntries = BYTESWAP_16(colorCount);

        // Convieniently, every palette format's pixel is 16-bit
        nextClutOffset += colorCount * 2;
    }

    // Texture Headers
    for (unsigned i = 0; i < textureCount; i++) {
        const TPL::TPLTexture& texture = this->textures[i];

        TPLHeader* header = headers + i;

        if (texture.width > 1024 || texture.height > 1024) {
            std::cerr <<
                "[TPLObject::Serialize] The dimensions of the texture exceed 1024x1024 "
                "(" << texture.width << 'x' << texture.height << "),\n"
                "                         the game will most likely be unable to read it properly.\n";
        }

        header->width = BYTESWAP_16(static_cast<uint16_t>(texture.width));
        header->height = BYTESWAP_16(static_cast<uint16_t>(texture.height));

        header->format = BYTESWAP_32(static_cast<uint32_t>(texture.format));

        // header->dataOffset gets set in the image data writing portion.

        if (
            texture.wrapS != TPL_WRAP_MODE_CLAMP &&
            !IS_POWER_OF_TWO(texture.width)
        ) {
            std::cerr << "[TPLObject::Serialize] Image width is not a power of two, so horizontal wrap will be set to CLAMP\n";
            header->wrapS = BYTESWAP_32(static_cast<uint32_t>(TPL_WRAP_MODE_CLAMP));
        }
        else
            header->wrapS = BYTESWAP_32(static_cast<uint32_t>(texture.wrapS));

        if (
            texture.wrapT != TPL_WRAP_MODE_CLAMP &&
            !IS_POWER_OF_TWO(texture.height)
        ) {
            std::cerr << "[TPLObject::Serialize] Image width is not a power of two, so vertical wrap will be set to CLAMP\n";
            header->wrapT = BYTESWAP_32(static_cast<uint32_t>(TPL_WRAP_MODE_CLAMP));
        }
        else
            header->wrapT = BYTESWAP_32(static_cast<uint32_t>(texture.wrapT));

        header->minFilter = BYTESWAP_32(static_cast<uint32_t>(texture.minFilter));
        header->magFilter = BYTESWAP_32(static_cast<uint32_t>(texture.magFilter));

        header->LODBias = 0.f;

        header->edgeLODEnable = 0;

        header->maxLOD = 0;
        header->minLOD = 0;
    }

    // Image Data

    unsigned writeOffset = dataSectionStart;
    for (unsigned i = 0; i < textureCount; i++) {
        TPL::TPLTexture& texture = this->textures[i];

        writeOffset = ALIGN_UP_32(writeOffset);
        headers[i].dataOffset = BYTESWAP_32(writeOffset);

        const unsigned imageSize = RvlImageConvert::getImageByteSize(texture);

        unsigned char* imageData = result.data() + writeOffset;
        RvlImageConvert::fromRGBA32(texture, imageData);

        auto it = std::find_if(
            paletteTextures.begin(), paletteTextures.end(),
            [i](const PaletteTexEntry& entry) {
                return entry.texIndex == i;
            }
        );
        if (it != paletteTextures.end()) {
            TPLClutHeader* clutHeader = clutHeaders + std::distance(paletteTextures.begin(), it);

            PaletteUtils::WriteRGBA32Palette(
                texture.palette.data(), texture.palette.size(),
                TPL_CLUT_FORMAT_RGB5A3,
                result.data() + BYTESWAP_32(clutHeader->dataOffset)
            );
        }

        writeOffset += imageSize;
    }

    return result;
}

} // namespace TPL
