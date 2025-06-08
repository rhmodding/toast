#include "TPL.hpp"

#include "../Logging.hpp"

#include "../MainThreadTaskManager.hpp"

#include "RvlImageConvert.hpp"
#include "RvlPalette.hpp"

#include "../Macro.hpp"

// 14 Feb 2000
// Pre-byteswapped to BE.
constexpr uint32_t TPL_VERSION_NUMBER = BYTESWAP_32(2142000);

struct TPLPalette {
    // Compare to TPL_VERSION_NUMBER
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
    uint16_t height; // Height of the texture in pixels.
    uint16_t width; // Width of the texture in pixels.

    uint32_t dataFormat; // The format of the image data (TPLImageFormat).
    uint32_t dataOffset; // File offset to the image data.

    uint32_t wrapS; // Horizontal UV wrapping mode (TPLWrapMode).
    uint32_t wrapT; // Vertical UV wrapping mode (TPLWrapMode).

    uint32_t minFilter; // Minification filter (TPLTexFilter).
    uint32_t magFilter; // Magnification filter (TPLTexFilter).

    float LODBias;

    uint8_t edgeLODEnable; // Not sure what this does.

    uint8_t minMipmap; // Index of minimum mipmap level. Usually zero.
    uint8_t maxMipmap; // Index of maximum mipmap level.

    // Set by game.
    uint8_t _relocated { 0x00 };
} __attribute__((packed));

struct TPLClutHeader {
    uint16_t numEntries;

    // Set by game.
    uint8_t _relocated { 0x00 };

    uint8_t _pad8 { 0x00 };

    uint32_t dataFormat; // The format of the color entries (TPLClutFormat).
    uint32_t dataOffset; // File offset to the color entries.
} __attribute__((packed));

// RGB5A3 is the only CLUT format with support for transparency & full color at
// the same time.
constexpr TPL::TPLClutFormat DEFAULT_CLUT_FORMAT = TPL::TPL_CLUT_FORMAT_RGB5A3;

namespace TPL {

GLuint TPLTexture::createGPUTexture() const {
    GLuint textureId { 0 };

    GLint minFilter, magFilter;

    switch (this->minFilter) {
    case TPL::TPL_TEX_FILTER_NEAR:
        minFilter = GL_NEAREST;
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
    case TPL::TPL_TEX_FILTER_LINEAR:
    default:
        minFilter = GL_LINEAR;
        break;
    }

    switch (this->magFilter) {
    case TPL::TPL_TEX_FILTER_NEAR:
        magFilter = GL_NEAREST;
        break;
    case TPL::TPL_TEX_FILTER_LINEAR:
    default:
        magFilter = GL_LINEAR;
        break;
    }

    MainThreadTaskManager::getInstance().QueueTask([this, &textureId, &minFilter, &magFilter]() {
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
            this->wrapS == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE
        );
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
            this->wrapT == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->data.data());

        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return textureId;
}

TPLObject::TPLObject(const unsigned char* tplData, const size_t dataSize) {
    if (dataSize < sizeof(TPLPalette)) {
        Logging::err << "[TPLObject::TPLObject] Invalid TPL binary: data size smaller than palette size!" << std::endl;
        return;
    }

    const TPLPalette* palette = reinterpret_cast<const TPLPalette*>(tplData);
    if (palette->versionNumber != TPL_VERSION_NUMBER) {
        Logging::err << "[TPLObject::TPLObject] Invalid TPL binary: invalid version number!" << std::endl;
        return;
    }

    const uint32_t descriptorCount = BYTESWAP_32(palette->descriptorCount);
    const TPLDescriptor* descriptors = reinterpret_cast<const TPLDescriptor*>(
        tplData + BYTESWAP_32(palette->descriptorsOffset)
    );

    mTextures.resize(descriptorCount);

    for (unsigned i = 0; i < descriptorCount; i++) {
        const TPLDescriptor* descriptor = descriptors + i;

        if (descriptor->textureHeaderOffset == 0) {
            Logging::err << "[TPLObject::TPLObject] Texture no. " << i+1 << " could not be read (texture header offset is zero)!" << std::endl;
            continue;
        }

        const TPLHeader* header = reinterpret_cast<const TPLHeader*>(
            tplData + BYTESWAP_32(descriptor->textureHeaderOffset)
        );

        TPLTexture& textureData = mTextures[i];

        if (descriptor->CLUTHeaderOffset != 0) {
            const TPLClutHeader* clutHeader = reinterpret_cast<const TPLClutHeader*>(
                tplData + BYTESWAP_32(descriptor->CLUTHeaderOffset)
            );

            RvlPalette::readCLUT(
                textureData.palette,

                tplData + BYTESWAP_32(clutHeader->dataOffset),
                BYTESWAP_16(clutHeader->numEntries),

                static_cast<TPLClutFormat>(BYTESWAP_32(clutHeader->dataFormat))
            );
        }

        textureData.width = BYTESWAP_16(header->width);
        textureData.height = BYTESWAP_16(header->height);

        textureData.format = static_cast<TPLImageFormat>(BYTESWAP_32(header->dataFormat));

        textureData.mipCount = header->maxMipmap - header->minMipmap + 1;

        textureData.wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(header->wrapS));
        textureData.wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(header->wrapT));

        textureData.minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header->minFilter));
        textureData.magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header->magFilter));

        const unsigned char* imageData = tplData + BYTESWAP_32(header->dataOffset);

        textureData.data.resize(textureData.width * textureData.height * 4);
        RvlImageConvert::toRGBA32(textureData, imageData);
    }

    mInitialized = true;
}

std::vector<unsigned char> TPLObject::Serialize() {
    std::vector<unsigned char> result;

    if (!mInitialized) {
        Logging::err << "[TPLObject::Serialize] Unable to serialize: not initialized!" << std::endl;
        return result;
    }

    const unsigned textureCount = mTextures.size();

    // Precompute required size & texture indexes for color palettes.
    unsigned paletteEntriesSize { 0 };

    struct PaletteTexEntry {
        unsigned texIndex;
        std::set<uint32_t> palette;
    };
    std::vector<PaletteTexEntry> paletteTextures;
    paletteTextures.reserve(textureCount);

    for (unsigned i = 0; i < textureCount; i++) {
        const auto& texture = mTextures[i];

        if (
            texture.format == TPL_IMAGE_FORMAT_C4 ||
            texture.format == TPL_IMAGE_FORMAT_C8 ||
            texture.format == TPL_IMAGE_FORMAT_C14X2
        ) {
            paletteTextures.push_back(PaletteTexEntry {
                .texIndex = i,
                .palette = RvlPalette::generate(
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

    // Precompute size of data section.
    for (unsigned i = 0; i < textureCount; i++) {
        const unsigned imageSize = RvlImageConvert::getImageByteSize(mTextures[i]);

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

        clutHeader->dataFormat = BYTESWAP_32(DEFAULT_CLUT_FORMAT);
        clutHeader->dataOffset = BYTESWAP_32(nextClutOffset);

        unsigned colorCount = ALIGN_UP_16(paletteTextures[clutIndex].palette.size());
        clutHeader->numEntries = BYTESWAP_16(colorCount);

        // Convieniently, every palette format's pixel is 16-bit
        nextClutOffset += colorCount * 2;
    }

    // Texture Headers
    for (unsigned i = 0; i < textureCount; i++) {
        const TPL::TPLTexture& texture = mTextures[i];

        TPLHeader* header = headers + i;

        if (texture.width > 1024 || texture.height > 1024) {
            Logging::warn << "[TPLObject::Serialize] The dimensions of the texture exceed 1024x1024 (" <<
                texture.width << 'x' << texture.height << ");" << std::endl;
            Logging::warn << "                       the game will most likely be unable to read it properly." << std::endl;
        }

        header->width = BYTESWAP_16(static_cast<uint16_t>(texture.width));
        header->height = BYTESWAP_16(static_cast<uint16_t>(texture.height));

        header->dataFormat = BYTESWAP_32(static_cast<uint32_t>(texture.format));

        // header->dataOffset gets set in the image data writing portion.

        if (
            texture.wrapS != TPL_WRAP_MODE_CLAMP &&
            !IS_POWER_OF_TWO(texture.width)
        ) {
            Logging::warn << "[TPLObject::Serialize] Image width is not a power of two, so horizontal wrap will be set to CLAMP" << std::endl;
            header->wrapS = BYTESWAP_32(static_cast<uint32_t>(TPL_WRAP_MODE_CLAMP));
        }
        else
            header->wrapS = BYTESWAP_32(static_cast<uint32_t>(texture.wrapS));

        if (
            texture.wrapT != TPL_WRAP_MODE_CLAMP &&
            !IS_POWER_OF_TWO(texture.height)
        ) {
            Logging::warn << "[TPLObject::Serialize] Image height is not a power of two, so vertical wrap will be set to CLAMP" << std::endl;
            header->wrapT = BYTESWAP_32(static_cast<uint32_t>(TPL_WRAP_MODE_CLAMP));
        }
        else
            header->wrapT = BYTESWAP_32(static_cast<uint32_t>(texture.wrapT));

        header->minFilter = BYTESWAP_32(static_cast<uint32_t>(texture.minFilter));
        header->magFilter = BYTESWAP_32(static_cast<uint32_t>(texture.magFilter));

        header->LODBias = 0.f;

        header->edgeLODEnable = 0;

        header->minMipmap = 0;
        header->maxMipmap = 0;
    }

    // Image Data

    unsigned writeOffset = dataSectionStart;
    for (unsigned i = 0; i < textureCount; i++) {
        TPL::TPLTexture& texture = mTextures[i];

        writeOffset = ALIGN_UP_32(writeOffset);
        headers[i].dataOffset = BYTESWAP_32(writeOffset);

        Logging::info <<
            "[TPLObject::Serialize] Writing data for texture no. " << (i+1) << " (" <<
            texture.width << 'x' << texture.height << ", " <<
            getImageFormatName(texture.format) << ").." << std::endl;

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

            RvlPalette::writeCLUT(
                result.data() + BYTESWAP_32(clutHeader->dataOffset),
                texture.palette, DEFAULT_CLUT_FORMAT
            );
        }

        writeOffset += imageSize;
    }

    return result;
}

} // namespace TPL
