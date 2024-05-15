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

    uint32_t format;
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
    TPLObject::TPLObject(const char* tplData, const size_t dataSize) {
        const TPLPalette* palette = reinterpret_cast<const TPLPalette*>(tplData);

        if (palette->versionNumber != TPL_VERSION_NUMBER) {
            std::cerr << "[TPLObject::TPLObject] Invalid TPL binary: invalid version number for texture palette!\n";
            return;
        }

        size_t readOffset = BYTESWAP_32(palette->descriptorsOffset);

        uint32_t descriptorCount = BYTESWAP_32(palette->descriptorCount);
        for (uint16_t i = 0; i < descriptorCount; i++) {
            TPLDescriptor descriptor;
            Common::ReadAtOffset(tplData, readOffset, dataSize, descriptor);

            size_t savedOffset = readOffset;

            TPLTexture textureData;

            if (descriptor.textureHeaderOffset != 0) {
                readOffset = BYTESWAP_32(descriptor.textureHeaderOffset);

                TPLHeader header;
                Common::ReadAtOffset(tplData, readOffset, dataSize, header);

                readOffset = BYTESWAP_32(descriptor.CLUTHeaderOffset);

                TPLClutHeader clutHeader;
                Common::ReadAtOffset(tplData, readOffset, dataSize, clutHeader);

                // Start handling
                textureData.width = BYTESWAP_16(header.width);
                textureData.height = BYTESWAP_16(header.height);

                TPLImageFormat format = static_cast<TPLImageFormat>(BYTESWAP_32(header.format));

                textureData.mipMap = header.minLOD == header.maxLOD ? 0 : 1;

                textureData.wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(header.wrapS));
                textureData.wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(header.wrapT));

                textureData.minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header.minFilter));
                textureData.magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(header.magFilter));

                // Read texture data
                std::vector<char> data;

                readOffset = BYTESWAP_32(header.dataOffset);

                size_t imageSize = ImageConvert::getImageByteSize(format, textureData.width, textureData.height);
                data.resize(imageSize);

                for (size_t j = 0; j < imageSize; j++) {
                    char c;
                    Common::ReadAtOffset(tplData, readOffset, dataSize, c);

                    data[j] = c;
                }

                ImageConvert::toRGBA32(textureData.data, format, textureData.width, textureData.height, data);

                textureData.valid = 0xFFu;
            }
            else
                textureData.valid = 0x00;

            this->textures.push_back(textureData);

            readOffset = savedOffset;
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

            TPLImageFormat format = TPL_IMAGE_FORMAT_RGBA32; // TODO: add options for different format

            // TODO: implement support for CLUT header writing

            header->width = BYTESWAP_16(this->textures.at(i).width);
            header->height = BYTESWAP_16(this->textures.at(i).height);

            header->format = BYTESWAP_32(TPL_IMAGE_FORMAT_RGBA32);

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