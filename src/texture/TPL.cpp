#include "TPL.hpp"

#include <iostream>
#include <fstream>

#include <unordered_set>

#include "../MtCommandManager.hpp"

#include "ImageConvert.hpp"

#include "PaletteUtils.hpp"

#include "../common.hpp"

#define TPL_VERSION_NUMBER 0x30AF2000

struct TPLClutHeader {
    uint16_t numEntries;

    // Boolean value, used in official SDK to keep track of status
    uint8_t unpacked;

    uint8_t pad8;

    // TPLClutFormat (big endian)
    uint32_t format;

    uint32_t dataOffset;
};

struct TPLPalette {
    uint32_t versionNumber; // 2142000 in big-endian. This is a revision date (Feb 14 2000)
    uint32_t descriptorCount;
    uint32_t descriptorsOffset;
};

struct TPLDescriptor {
    uint32_t textureHeaderOffset;
    uint32_t CLUTHeaderOffset;
};

struct TPLHeader {
    uint16_t height;
    uint16_t width;

    // TPLImageFormat (big endian)
    uint32_t format;

    uint32_t dataOffset;

    TPL::TPLWrapMode wrapS;
    TPL::TPLWrapMode wrapT;

    TPL::TPLTexFilter minFilter;
    TPL::TPLTexFilter magFilter;

    float LODBias;

    // Boolean value
    uint8_t edgeLODEnable;

    uint8_t minLOD;
    uint8_t maxLOD;

    // Boolean value, used in official SDK to keep track of status
    uint8_t unpacked;
};

namespace TPL {
    const char* getImageFormatName(TPLImageFormat format) {
        const char* strings[TPL_IMAGE_FORMAT_COUNT] = {
            "I4",      // TPL_IMAGE_FORMAT_I4
            "I8",      // TPL_IMAGE_FORMAT_I8
            "IA4",     // TPL_IMAGE_FORMAT_IA4
            "IA8",     // TPL_IMAGE_FORMAT_IA8
            "RGB565",  // TPL_IMAGE_FORMAT_RGB565
            "RGB5A3",  // TPL_IMAGE_FORMAT_RGB5A3
            "RGBA32",  // TPL_IMAGE_FORMAT_RGBA32
            nullptr,   // Index 7 (unused)
            "C4",      // TPL_IMAGE_FORMAT_C4 (0x08)
            "C8",      // TPL_IMAGE_FORMAT_C8
            "C14X2",   // TPL_IMAGE_FORMAT_C14X2
            nullptr,   // Index 11 (unused)
            nullptr,   // Index 12 (unused)
            nullptr,   // Index 13 (unused)
            "CMPR"     // TPL_IMAGE_FORMAT_CMPR (0x0E)
        };

        if (format >= TPL_IMAGE_FORMAT_COUNT)
            return "Unknown format";

        return strings[format];
    }

    TPLObject::TPLObject(const unsigned char* tplData, const size_t dataSize) {
        if (dataSize < sizeof(TPLPalette)) {
            std::cerr << "[TPLObject::TPLObject] Invalid TPL binary: data size smaller than palette size!\n";
            return;
        }

        const TPLPalette* palette = reinterpret_cast<const TPLPalette*>(tplData);

        if (UNLIKELY(palette->versionNumber != TPL_VERSION_NUMBER)) {
            std::cerr << "[TPLObject::TPLObject] Invalid TPL binary: invalid version number for texture palette!\n";
            return;
        }

        uint32_t descriptorCount = BYTESWAP_32(palette->descriptorCount);
        for (uint16_t i = 0; i < descriptorCount; i++) {
            const TPLDescriptor* descriptor = reinterpret_cast<const TPLDescriptor*>(
                tplData + BYTESWAP_32(palette->descriptorsOffset) +
                (sizeof(TPLDescriptor) * i)
            );

            TPLTexture textureData;

            if (descriptor->textureHeaderOffset != 0) {
                const TPLHeader* header = reinterpret_cast<const TPLHeader*>(tplData + BYTESWAP_32(descriptor->textureHeaderOffset));

                if (descriptor->CLUTHeaderOffset != 0) {
                    const TPLClutHeader* clutHeader = reinterpret_cast<const TPLClutHeader*>(tplData + BYTESWAP_32(descriptor->CLUTHeaderOffset));

                    PaletteUtils::CLUTtoRGBA32Palette(
                        textureData.palette,

                        reinterpret_cast<const uint8_t*>(tplData + BYTESWAP_32(clutHeader->dataOffset)),
                        BYTESWAP_16(clutHeader->numEntries),

                        static_cast<TPL::TPLClutFormat>(BYTESWAP_32(clutHeader->format))
                    );
                }

                // Start handling
                textureData.width = BYTESWAP_16(header->width);
                textureData.height = BYTESWAP_16(header->height);

                TPLImageFormat format = static_cast<TPLImageFormat>(BYTESWAP_32(header->format));
                textureData.format = format;

                textureData.mipMap = header->minLOD == header->maxLOD ? 0 : 1;

                textureData.wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(header->wrapS));
                textureData.wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(header->wrapT));

                textureData.minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header->minFilter));
                textureData.magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header->magFilter));

                uint32_t imageSize = ImageConvert::getImageByteSize(textureData);
                const unsigned char* imageData = tplData + BYTESWAP_32(header->dataOffset);

                ImageConvert::toRGBA32(
                    textureData.data,
                    format,
                    textureData.width, textureData.height,
                    imageData,
                    textureData.palette.data()
                );

                textureData.valid = 0xFFu;
            }
            else
                textureData.valid = 0x00;

            this->textures.push_back(textureData);
        }
    }

    std::vector<unsigned char> TPLObject::Reserialize() {
        std::vector<unsigned char> result;

        // Precompute size required & texture indexes for color palettes
        uint32_t paletteEntriesSize{ 0 };
        std::unordered_set<uint32_t> paletteTextures;
        for (uint32_t i = 0; i < this->textures.size(); i++) {
            if (
                this->textures.at(i).format == TPL_IMAGE_FORMAT_C4 ||
                this->textures.at(i).format == TPL_IMAGE_FORMAT_C8 ||
                this->textures.at(i).format == TPL_IMAGE_FORMAT_C14X2
            ) {
                paletteTextures.insert(i);

                // All palette formats have a consistent pixel size of 16 bits
                paletteEntriesSize += (this->textures.at(i).palette.size() * sizeof(uint16_t));
            }
        }

        uint32_t baseSize = (
            ((
                sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * this->textures.size()) +
                (sizeof(TPLClutHeader) * paletteTextures.size()) +
                31
            ) & ~31) + // Align to 32 bytes
            paletteEntriesSize +
            (sizeof(TPLHeader) * this->textures.size())
        );

        result.resize(baseSize);

        TPLPalette* palette = reinterpret_cast<TPLPalette*>(result.data());
        palette->versionNumber = TPL_VERSION_NUMBER;
        palette->descriptorCount = BYTESWAP_32(static_cast<uint32_t>(this->textures.size()));
        palette->descriptorsOffset = BYTESWAP_32(sizeof(TPLPalette));

        uint8_t* paletteWritePtr = nullptr;
        TPLHeader* headers = nullptr;

        if (!paletteTextures.empty()) {
            paletteWritePtr = reinterpret_cast<uint8_t*>(
                result.data() + (
                    (
                        sizeof(TPLPalette) +
                        (sizeof(TPLDescriptor) * this->textures.size()) +
                        (sizeof(TPLClutHeader) * paletteTextures.size())
                        + 31
                    ) & ~31
                )
            );
        }

        headers = reinterpret_cast<TPLHeader*>(
            result.data() +
            baseSize -
            (sizeof(TPLHeader) * this->textures.size())
        );

        // Texture Descriptors
        for (uint32_t textureIndex = 0, clutIndex = 0; textureIndex < this->textures.size(); textureIndex++) {
            TPLDescriptor* descriptor = reinterpret_cast<TPLDescriptor*>(
                result.data() +
                sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * textureIndex)
            );

            descriptor->textureHeaderOffset = BYTESWAP_32(static_cast<uint32_t>((
                (
                    sizeof(TPLPalette) +
                    (sizeof(TPLDescriptor) * this->textures.size()) +
                    (sizeof(TPLClutHeader) * paletteTextures.size()) +
                    31
                ) & ~31) + // Align to 32 bytes
                paletteEntriesSize +
                (sizeof(TPLHeader) * textureIndex)
            ));

            if (paletteTextures.find(textureIndex) != paletteTextures.end()) {
                descriptor->CLUTHeaderOffset = BYTESWAP_32(static_cast<uint32_t>(
                    sizeof(TPLPalette) +
                    (sizeof(TPLDescriptor) * this->textures.size()) +
                    (sizeof(TPLClutHeader) * clutIndex++)
                ));
            }
            else {
                descriptor->CLUTHeaderOffset = 0x00000000;
            }
        }

        // Palette Descriptors & Data
        for (uint32_t textureIndex = 0, clutIndex = 0; textureIndex < this->textures.size(); textureIndex++) {
            if (paletteTextures.find(textureIndex) == paletteTextures.end()) // No need to write a palette descriptor.
                continue;

            TPLClutHeader* paletteDescriptor = reinterpret_cast<TPLClutHeader*>(
                result.data() +
                sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * this->textures.size()) +
                (sizeof(TPLClutHeader) * clutIndex)
            );

            paletteDescriptor->unpacked = 0x00;

            paletteDescriptor->format = BYTESWAP_32(TPL::TPL_CLUT_FORMAT_RGB5A3);

            // Data
            {
                paletteDescriptor->dataOffset = BYTESWAP_32(static_cast<uint32_t>(
                    paletteWritePtr - result.data()
                ));

                paletteDescriptor->numEntries = BYTESWAP_16(this->textures.at(textureIndex).palette.size());

                PaletteUtils::WriteRGBA32Palette(
                    this->textures.at(textureIndex).palette.data(),
                    this->textures.at(textureIndex).palette.size(),
                    TPL::TPL_CLUT_FORMAT_RGB5A3,
                    paletteWritePtr
                );

                paletteWritePtr += (this->textures.at(textureIndex).palette.size() * sizeof(uint16_t));
            }

            clutIndex++;
        }

        // Texture Headers
        for (uint32_t i = 0; i < this->textures.size(); i++) {
            TPL::TPLTexture& texture = this->textures.at(i);

            TPLHeader* header = reinterpret_cast<TPLHeader*>(
                result.data() +
                baseSize -
                (sizeof(TPLHeader) * this->textures.size()) +
                (sizeof(TPLHeader) * i)
            );

            header->width = BYTESWAP_16(texture.width);
            header->height = BYTESWAP_16(texture.height);

            header->format = BYTESWAP_32(texture.format);

            // header->dataOffset gets set in the image data writing portion.

            header->wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(texture.wrapS));
            header->wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(texture.wrapT));

            header->minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(texture.minFilter));
            header->magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(texture.magFilter));

            header->LODBias = 0.f;

            header->edgeLODEnable = 0x00;

            header->maxLOD = 0x00;
            header->minLOD = 0x00;

            header->unpacked = 0x00;
        }

        // Image Data
        {
            uint32_t writeOffset = static_cast<uint32_t>(result.size());

            for (uint32_t i = 0; i < this->textures.size(); i++) {
                TPL::TPLTexture& texture = this->textures.at(i);

                // Align writeOffset to 64 bytes
                writeOffset = (writeOffset + 0x3F) & ~0x3F;

                // Write header's dataOffset
                (reinterpret_cast<TPLHeader*>(
                    result.data() +
                    baseSize -
                    (sizeof(TPLHeader) * this->textures.size())
                ) + i)->dataOffset = BYTESWAP_32(writeOffset);

                std::vector<unsigned char> imageData;
                ImageConvert::fromRGBA32(
                    imageData,
                    texture.format,
                    texture.width, texture.height,
                    texture.data.data(),
                    &texture.palette
                );

                result.resize(writeOffset + imageData.size());
                memcpy(result.data() + writeOffset, imageData.data(), imageData.size());

                writeOffset += imageData.size();
            }
        }

        return result;
    }

    std::optional<TPLObject> readTPLFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (UNLIKELY(!file.is_open())) {
            std::cerr << "[TPL::readTPLFile] Error opening file at path: " << filePath << '\n';
            return std::nullopt;
        }

        // File read
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> fileContent(fileSize);
        file.read(fileContent.data(), fileSize);

        file.close();

        TPL::TPLObject tpl(
            reinterpret_cast<unsigned char*>(fileContent.data()),
            fileSize
        );

        return tpl;
    }

    GLuint LoadTPLTextureIntoGLTexture(const TPL::TPLTexture& tplTexture) {
        GLuint imageTexture;

        GLint minFilter{ GL_LINEAR };
        GLint magFilter{ GL_LINEAR };

        TPL::TPLTexFilter tplMinFilter = tplTexture.minFilter;
        TPL::TPLTexFilter tplMagFilter = tplTexture.magFilter;

        std::future<void> future = MtCommandManager::getInstance().enqueueCommand([&imageTexture, &tplTexture, &minFilter, &magFilter]() {
            glGenTextures(1, &imageTexture);
            glBindTexture(GL_TEXTURE_2D, imageTexture);

            switch (tplTexture.minFilter) {
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

            switch (tplTexture.magFilter) {
                case TPL::TPL_TEX_FILTER_NEAR:
                    magFilter = GL_NEAREST;
                    break;
                case TPL::TPL_TEX_FILTER_LINEAR:
                    break;
                default:
                    break;
            }

            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                tplTexture.wrapS == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
            );
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                tplTexture.wrapT == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tplTexture.width, tplTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());

            glBindTexture(GL_TEXTURE_2D, 0);
        });

        future.get();

        return imageTexture;
    }
}
