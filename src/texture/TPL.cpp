#include "TPL.hpp"

#include "../common.hpp"

#include <iostream>
#include <fstream>

#include "ImageConvert.hpp"

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

        if (BYTESWAP_32(palette->versionNumber) != 0x0020AF30) {
            std::cerr << "[TPLObject::TPLObject] Invalid TPL binary: invalid version number for texture palette!\n";
            return;
        }

        size_t readOffset = BYTESWAP_32(palette->descriptorsOffset);

        uint32_t descriptorCount = BYTESWAP_32(palette->descriptorCount);
        for (uint16_t i = 0; i < descriptorCount; i++) {
            TPLDescriptor descriptor;
            Common::ReadAtOffset(tplData, readOffset, dataSize, descriptor);

            size_t savedOffset = readOffset;

            if (descriptor.textureHeaderOffset != 0) {
                readOffset = BYTESWAP_32(descriptor.textureHeaderOffset);

                TPLHeader header;
                Common::ReadAtOffset(tplData, readOffset, dataSize, header);

                readOffset = BYTESWAP_32(descriptor.CLUTHeaderOffset);

                TPLClutHeader clutHeader;
                Common::ReadAtOffset(tplData, readOffset, dataSize, clutHeader);

                // Start handling
                TPLTexture textureData;

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
                this->textures.push_back(textureData);
            }

            readOffset = savedOffset;
        }
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
}