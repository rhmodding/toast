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

        uint32_t writeOffset{ sizeof(TPLPalette) };

        std::vector<TPLDescriptor*> descriptors;
        for (uint32_t i = 0; i < this->textures.size(); i++) {
            TPLDescriptor* descriptor = reinterpret_cast<TPLDescriptor*>(result.data() + writeOffset);
            writeOffset += sizeof(TPLDescriptor);

            descriptor->CLUTHeaderOffset = 0x00000000;
            descriptor->textureHeaderOffset = 0x00000000;

            descriptors.push_back(descriptor);
        }

        for (uint32_t i = 0; i < this->textures.size(); i++) {
            TPLDescriptor* descriptor = descriptors.at(i);
            if (!this->textures.at(i).valid) 
                continue;

            descriptor->textureHeaderOffset = BYTESWAP_32(writeOffset);

            TPLHeader* header = reinterpret_cast<TPLHeader*>(result.data() + writeOffset);
            writeOffset += sizeof(TPLHeader);

            TPLImageFormat format = TPL_IMAGE_FORMAT_RGBA32; // TODO: add options for different format
            bool hasPalette =
                (format == TPL_IMAGE_FORMAT_C4) ||
                (format == TPL_IMAGE_FORMAT_C8) ||
                (format == TPL_IMAGE_FORMAT_C14X2);

            // CLUT Header
            TPLClutHeader* clutHeader{ nullptr };
            if (hasPalette) {
                result.resize(result.size() + sizeof(TPLClutHeader));

                descriptor->CLUTHeaderOffset = BYTESWAP_32(writeOffset);

                clutHeader = reinterpret_cast<TPLClutHeader*>(result.data() + writeOffset);
                writeOffset += sizeof(TPLClutHeader);

                // TODO: implement
            }

            header->width = BYTESWAP_16(this->textures.at(i).width);
            header->height = BYTESWAP_16(this->textures.at(i).height);

            header->format = BYTESWAP_32(TPL_IMAGE_FORMAT_RGBA32);

            header->dataOffset = BYTESWAP_32(writeOffset);

            header->wrapS = static_cast<TPLWrapMode>(BYTESWAP_32(this->textures.at(i).wrapS));
            header->wrapT = static_cast<TPLWrapMode>(BYTESWAP_32(this->textures.at(i).wrapT));

            header->minFilter = static_cast<TPLTexFilter>(BYTESWAP_32(this->textures.at(i).minFilter));
            header->magFilter = static_cast<TPLTexFilter>(BYTESWAP_32(this->textures.at(i).magFilter));

            header->LODBias = 0.f;

            header->edgeLODEnable = 0x00;

            header->maxLOD = 0x01;
            header->minLOD = 0x01;

            header->unpacked = 0x00;

            // Image Data
            header->dataOffset = BYTESWAP_32(writeOffset);

            uint32_t imageSize = static_cast<uint32_t>(ImageConvert::getImageByteSize(
                format, this->textures.at(i).width, this->textures.at(i).height
            ));
            result.resize(result.size() + imageSize);

            std::vector<char> imageData(imageSize);
            ImageConvert::fromRGBA32(
                imageData,
                format,
                this->textures.at(i).width,
                this->textures.at(i).height,
                this->textures.at(i).data
            );

            memcpy(result.data() + writeOffset, imageData.data(), imageData.size());

            writeOffset += static_cast<uint32_t>(imageData.size());
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
}