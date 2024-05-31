#include "TPL.hpp"

#include "../common.hpp"

#include <iostream>
#include <fstream>

#include "ImageConvert.hpp"

#define TPL_VERSION_NUMBER 0x30AF2000

struct TPLClutHeader {
    uint16_t numEntries;
    uint8_t unpacked;

    uint8_t pad8;

    TPL::TPLClutFormat format;
    
    uint32_t dataOffset;
};

struct TPLPalette {
    uint32_t versionNumber;
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

    TPLObject::TPLObject(const char* tplData, const size_t dataSize) {
        const TPLPalette* palette = reinterpret_cast<const TPLPalette*>(tplData);

        if (palette->versionNumber != TPL_VERSION_NUMBER) {
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

                uint32_t* colorPalette{ nullptr };
                if (descriptor->CLUTHeaderOffset != 0) {
                    const TPLClutHeader* clutHeader = reinterpret_cast<const TPLClutHeader*>(tplData + BYTESWAP_32(descriptor->CLUTHeaderOffset));

                    colorPalette = new uint32_t[BYTESWAP_16(clutHeader->numEntries)];

                    // TODO: this should probably be moved to ImageConvert

                    const uint8_t* data = reinterpret_cast<const uint8_t*>(tplData + BYTESWAP_32(clutHeader->dataOffset));
                    switch (BYTESWAP_32(clutHeader->format)) {
                        case TPL_CLUT_FORMAT_IA8: {
                            for (uint16_t i = 0; i < BYTESWAP_16(clutHeader->numEntries); i++) {
                                const uint8_t alpha = data[(i * 2) + 0];
                                const uint8_t intensity = data[(i * 2) + 1];

                                colorPalette[i] =
                                    (uint32_t(intensity) << 24) |
                                    (uint32_t(intensity) << 16) |
                                    (uint32_t(intensity) << 8) |
                                    uint32_t(alpha);
                            }
                        } break;
                        case TPL_CLUT_FORMAT_RGB565: {
                            for (uint16_t i = 0; i < BYTESWAP_16(clutHeader->numEntries); i++) {
                                const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data + (i * 2)));

                                colorPalette[i] =
                                    (uint32_t(((sourcePixel >> 11) & 0x1f) << 3) << 24) |
                                    (uint32_t(((sourcePixel >>  5) & 0x3f) << 2) << 16) |
                                    (uint32_t(((sourcePixel >>  0) & 0x1f) << 3) << 8) |
                                    0xFFu;
                            }
                        } break;
                        case TPL_CLUT_FORMAT_RGB5A3: {
                            for (uint16_t i = 0; i < BYTESWAP_16(clutHeader->numEntries); i++) {
                                const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data + (i * 2)));

                                uint8_t r, g, b, a;

                                if ((sourcePixel & 0x8000) != 0) { // RGB555
                                    r = (((sourcePixel >> 10) & 0x1f) << 3) | (((sourcePixel >> 10) & 0x1f) >> 2);
                                    g = (((sourcePixel >> 5) & 0x1f) << 3) | (((sourcePixel >> 5) & 0x1f) >> 2);
                                    b = (((sourcePixel) & 0x1f) << 3) | (((sourcePixel) & 0x1f) >> 2);

                                    a = 0xFFu;
                                }
                                else { // RGBA4443
                                    a =
                                        (((sourcePixel >> 12) & 0x7) << 5) | (((sourcePixel >> 12) & 0x7) << 2) |
                                        (((sourcePixel >> 12) & 0x7) >> 1);

                                    r = (((sourcePixel >> 8) & 0xf) << 4) | ((sourcePixel >> 8) & 0xf);
                                    g = (((sourcePixel >> 4) & 0xf) << 4) | ((sourcePixel >> 4) & 0xf);
                                    b = (((sourcePixel) & 0xf) << 4) | ((sourcePixel) & 0xf);
                                }

                                colorPalette[i] =
                                    (uint32_t(r) << 24) |
                                    (uint32_t(g) << 16) |
                                    (uint32_t(b) << 8) |
                                    uint32_t(a);
                            }
                        } break;
                        
                        default:
                            std::cerr <<
                                "[TPLObject::TPLObject] Invalid CLUT header: invalid color palette format! Expected 0 to 2, got " <<
                                BYTESWAP_32(clutHeader->format) << '\n';
                            break;
                    }
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


                uint32_t imageSize = ImageConvert::getImageByteSize(format, textureData.width, textureData.height);
                const char* imageData = tplData + BYTESWAP_32(header->dataOffset);

                std::vector<char> data(imageData, imageData + imageSize);
                ImageConvert::toRGBA32(textureData.data, format, textureData.width, textureData.height, data, colorPalette);

                if (colorPalette)
                    delete[] colorPalette;

                textureData.valid = 0xFFu;
            }
            else
                textureData.valid = 0x00;

            this->textures.push_back(textureData);
        }
    }

    std::vector<char> TPLObject::Reserialize() {
        std::vector<char> result(
            sizeof(TPLPalette) +
            ((sizeof(TPLDescriptor) + sizeof(TPLHeader)) * this->textures.size())
        );

        TPLPalette* palette = reinterpret_cast<TPLPalette*>(result.data());
        palette->versionNumber = TPL_VERSION_NUMBER;
        palette->descriptorCount = BYTESWAP_32(static_cast<uint32_t>(this->textures.size()));
        palette->descriptorsOffset = BYTESWAP_32(sizeof(TPLPalette));

        // Write descriptors and headers
        for (uint32_t i = 0; i < this->textures.size(); i++) {
            TPLDescriptor* descriptor = reinterpret_cast<TPLDescriptor*>(
                result.data() + sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * i)
            );
            TPLHeader* header = reinterpret_cast<TPLHeader*>(
                result.data() + sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * this->textures.size()) +
                (sizeof(TPLHeader) * i)
            );

            if (!this->textures.at(i).valid) {
                // Original texture couldn't be read properly, makes no sense to write it
                descriptor->CLUTHeaderOffset = 0x00000000;
                descriptor->textureHeaderOffset = 0x00000000;
                continue;
            }

            descriptor->CLUTHeaderOffset = 0x00000000;
            descriptor->textureHeaderOffset = BYTESWAP_32(static_cast<uint32_t>(
                reinterpret_cast<char*>(header) - result.data()
            ));

            // TODO: implement support for CLUT header writing

            header->width = BYTESWAP_16(this->textures.at(i).width);
            header->height = BYTESWAP_16(this->textures.at(i).height);

            header->format = BYTESWAP_32(this->textures.at(i).format);

            // header->dataOffset gets set in the image data writing portion.

            header->wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(this->textures.at(i).wrapS));
            header->wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(this->textures.at(i).wrapT));

            header->minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(this->textures.at(i).minFilter));
            header->magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(this->textures.at(i).magFilter));

            header->LODBias = 0.f;

            header->edgeLODEnable = 0x00;

            header->maxLOD = 0x00;
            header->minLOD = 0x00;

            header->unpacked = 0x00;
        }

        // Image data write offset
        uint32_t writeOffset = static_cast<uint32_t>(
            sizeof(TPLPalette) +
            ((sizeof(TPLDescriptor) + sizeof(TPLHeader)) * this->textures.size())
        );

        // Write image data.
        // Note: result is dynamically resized in this portion.
        for (uint32_t i = 0; i < this->textures.size(); i++) {
            TPLHeader* header = reinterpret_cast<TPLHeader*>(
                result.data() + sizeof(TPLPalette) +
                (sizeof(TPLDescriptor) * this->textures.size()) +
                (sizeof(TPLHeader) * i)
            );

            // Align writeOffset to 64 bytes
            writeOffset = (writeOffset + 0x3F) & ~0x3F;

            // Set data offset
            header->dataOffset = BYTESWAP_32(writeOffset);

            uint32_t imageSize = ImageConvert::getImageByteSize(
                static_cast<TPL::TPLImageFormat>(BYTESWAP_32(header->format)),
                this->textures.at(i).width, this->textures.at(i).height
            );

            std::vector<char> imageData(imageSize);
            ImageConvert::fromRGBA32(
                imageData,
                static_cast<TPL::TPLImageFormat>(BYTESWAP_32(header->format)),
                this->textures.at(i).width,
                this->textures.at(i).height,
                this->textures.at(i).data
            );

            result.resize(writeOffset + imageSize);
            memcpy(result.data() + writeOffset, imageData.data(), imageData.size());

            writeOffset += imageSize;
        }

        return result;
    }

    std::optional<TPLObject> readTPLFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
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

        TPL::TPLObject tpl(fileContent.data(), fileSize);

        return tpl;
    }

    GLuint LoadTPLTextureIntoGLTexture(TPL::TPLTexture tplTexture) {
        GLuint imageTexture;
        
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        GLint minFilter;
        GLint magFilter;

        switch (tplTexture.magFilter) {
            case TPL::TPL_TEX_FILTER_NEAR:
                magFilter = GL_NEAREST;
                break;
            case TPL::TPL_TEX_FILTER_LINEAR:
                magFilter = GL_LINEAR;
                break;
            default:
                magFilter = GL_LINEAR;
                break;
        }

        switch (tplTexture.minFilter) {
            case TPL::TPL_TEX_FILTER_NEAR:
                minFilter = GL_NEAREST;
                break;
            case TPL::TPL_TEX_FILTER_LINEAR:
                minFilter = GL_LINEAR;
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
                minFilter = GL_LINEAR;
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
        
        return imageTexture;
    }
}