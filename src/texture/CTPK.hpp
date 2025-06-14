#ifndef CTPK_HPP
#define CTPK_HPP

#include <cstddef>

#include <cstdint>

#include <vector>

#include <string>

#include "glInclude.hpp"

namespace CTPK {

enum CTPKImageFormat : uint32_t {
    // Color + Alpha (24-bit truecolor + 8-bit alpha)
    CTPK_IMAGE_FORMAT_RGBA8888,

    // Color (24-bit truecolor)
    CTPK_IMAGE_FORMAT_RGB888,

    // Color + Alpha (5 bits per channel + 1-bit alpha)
    CTPK_IMAGE_FORMAT_RGBA5551,

    // Color (5 bits for red, 6 bits for green, 5 bits for blue)
    CTPK_IMAGE_FORMAT_RGB565,

    // Color + Alpha (4 bits per channel)
    CTPK_IMAGE_FORMAT_RGBA4444,

    // Grayscale + Alpha (8 bits for each)
    CTPK_IMAGE_FORMAT_LA88,

    // 2-channel (4 bits for red and green)
    CTPK_IMAGE_FORMAT_HL8,

    // Grayscale (8 bits)
    CTPK_IMAGE_FORMAT_L8,

    // Alpha (8 bits)
    CTPK_IMAGE_FORMAT_A8,

    // Grayscale + Alpha (4 bits for each)
    CTPK_IMAGE_FORMAT_LA44,

    // Grayscale (4 bits)
    CTPK_IMAGE_FORMAT_L4,

    // Alpha (4 bits)
    CTPK_IMAGE_FORMAT_A4,

    // Color (ETC1)
    CTPK_IMAGE_FORMAT_ETC1,

    // Color + Alpha (ETC1)
    CTPK_IMAGE_FORMAT_ETC1A4,

    CTPK_IMAGE_FORMAT_COUNT
};

constexpr const char* getImageFormatName(CTPKImageFormat format) {
    constexpr const char* strings[CTPK_IMAGE_FORMAT_COUNT] = {
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
        "ETC1A4"    // CTPK_IMAGE_FORMAT_ETC1A4
    };

    if (format >= CTPK_IMAGE_FORMAT_COUNT)
        return "Invalid format";

    return strings[format];
}

constexpr bool getImageFormatCompressed(CTPKImageFormat format) {
    return
        format == CTPK_IMAGE_FORMAT_ETC1 ||
        format == CTPK_IMAGE_FORMAT_ETC1A4;
}

struct CTPKTexture {
public:
    unsigned width;
    unsigned height;

    unsigned mipCount;

    CTPKImageFormat targetFormat;

    uint32_t sourceTimestamp; // Unix last modified timestamp of source TGA.
    std::string sourcePath; // Path of source TGA.

    // In targeted format (see targetFormat field). Includes mipmaps.
    std::vector<unsigned char> cachedTargetData;

    std::vector<unsigned char> data; // In RGBA32 format.

public:
    void rotateCCW();
    void rotateCW();

    [[nodiscard]] GLuint createGPUTexture() const;
};

class CTPKObject {
public:
    CTPKObject(const unsigned char* ctpkData, const size_t dataSize);
    CTPKObject() :
        mInitialized(true)
    {};

    bool isInitialized() const { return mInitialized; }

    [[nodiscard]] std::vector<unsigned char> Serialize();

public:
    bool mInitialized { false };

    std::vector<CTPKTexture> mTextures;
};

} // namespace CTPK

#endif // CTPK_HPP
