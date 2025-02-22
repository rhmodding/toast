#include "CTPK.hpp"

#include "../MainThreadTaskManager.hpp"

#include "CtrImageConvert.hpp"

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

    uint32_t dimensionsOffset; // Offset to this->width, so always 0x10.

    uint32_t sourceTimestamp; // Unix timestamp of source texture.
} __attribute((packed));

struct CtpkFileHeader {
    uint32_t magic; // Compare to CTPK_MAGIC
    uint16_t formatVersion; // Compare to CTPK_VERSION

    uint16_t textureCount;

    uint32_t dataSectionOffset; // Offset to the data section.
    uint32_t dataSectionSize; // Size of the data section.

    uint32_t lookupSectionOffset; // Offset to the lookup section.

    // Note: this section is pretty much useless, the only unique data given
    //       here is the compression quality.
    uint32_t infoSectionOffset; // Offset to the info section.

    uint32_t _reserved[2];

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
    uint8_t compressionQuality; 
} __attribute((packed));

namespace CTPK {

const char* getImageFormatName(CTPKImageFormat format) {
    const char* strings[CTPK_IMAGE_FORMAT_COUNT] = {
        "RGBA8888", // CTPK_IMAGE_FORMAT_RGBA8888
        "RGB888",   // CTPK_IMAGE_FORMAT_RGB888
        "RGB5551",  // CTPK_IMAGE_FORMAT_RGBA5551
        "RGB565",   // CTPK_IMAGE_FORMAT_RGB565
        "RGBA4444", // CTPK_IMAGE_FORMAT_RGBA4444
        "LA88",     // CTPK_IMAGE_FORMAT_LA88
        "HL8",      // CTPK_IMAGE_FORMAT_HL8
        "L8",       // CTPK_IMAGE_FORMAT_L8
        "A8",       // CTPK_IMAGE_FORMAT_A8
        "LA44",     // CTPK_IMAGE_FORMAT_LA44
        "L4",       // CTPK_IMAGE_FORMAT_L4
        "A4",       // CTPK_IMAGE_FORMAT_A4
        "ETC1",     // CTPK_IMAGE_FORMAT_ETC1
        "ETC1A4"   // CTPK_IMAGE_FORMAT_ETC1A4
    };

    if (format >= CTPK_IMAGE_FORMAT_COUNT)
        return "Invalid format";

    return strings[format];
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
    std::vector<unsigned char> result;

    // TODO: implement

    return result;
}

}
