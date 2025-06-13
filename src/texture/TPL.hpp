#ifndef TPL_HPP
#define TPL_HPP

#include <cstdint>

#include <vector>

#include "glInclude.hpp"

namespace TPL {

enum TPLWrapMode {
    TPL_WRAP_MODE_CLAMP,
    TPL_WRAP_MODE_REPEAT,
    TPL_WRAP_MODE_MIRROR
};

enum TPLTexFilter {
    TPL_TEX_FILTER_NEAR,
    TPL_TEX_FILTER_LINEAR,
    TPL_TEX_FILTER_NEAR_MIP_NEAR,
    TPL_TEX_FILTER_LIN_MIP_NEAR,
    TPL_TEX_FILTER_NEAR_MIP_LIN,
    TPL_TEX_FILTER_LIN_MIP_LIN
};

// https://wiki.tockdom.com/wiki/Image_Formats
enum TPLImageFormat : uint32_t {
    // Grayscale (4 bits)
    TPL_IMAGE_FORMAT_I4,

    // Grayscale (8 bits)
    TPL_IMAGE_FORMAT_I8,

    // Grayscale + Alpha (4 bits for each)
    TPL_IMAGE_FORMAT_IA4,

    // Grayscale + Alpha (8 bits for each)
    TPL_IMAGE_FORMAT_IA8,

    // Color (5 bits for red, 6 bits for green, 5 bits for blue)
    TPL_IMAGE_FORMAT_RGB565,

    // Color + Alpha (16-bit pixels)
    //   - Pixel type A: 5 bits for each color
    //   - Pixel type B: 4 bits for each color, 3 bit alpha
    TPL_IMAGE_FORMAT_RGB5A3,

    // Color + Alpha (24-bit truecolor + 8-bit alpha)
    TPL_IMAGE_FORMAT_RGBA32,

    // Palette (Indexed 4-bit pixels from IA8, RGB565, or RGB5A3)
    TPL_IMAGE_FORMAT_C4 = 0x08,

    // Palette (Indexed 8-bit pixels from IA8, RGB565, or RGB5A3)
    TPL_IMAGE_FORMAT_C8,

    // Palette (Indexed 16-bit pixels from IA8, RGB565, or RGB5A3)
    //   - The index is actually 14 bits since the top 2 bits are ignored.
    TPL_IMAGE_FORMAT_C14X2,

    // Color + Alpha (DXT1)
    TPL_IMAGE_FORMAT_CMPR = 0x0E,

    TPL_IMAGE_FORMAT_COUNT
};

enum TPLClutFormat : uint32_t {
    // Grayscale + Alpha (8 bits for each)
    TPL_CLUT_FORMAT_IA8,

    // Color (5 bits for red, 6 bits for green, 5 bits for blue)
    TPL_CLUT_FORMAT_RGB565,

    // Color + Alpha (16-bit pixels with variable modes)
    //   - Pixel mode A: 5 bits for red, green and blue
    //   - Pixel mode B: 4 bits for red, green, and blue + 3 bit alpha
    TPL_CLUT_FORMAT_RGB5A3,

    TPL_CLUT_FORMAT_COUNT
};

constexpr const char* getImageFormatName(TPLImageFormat format) {
    constexpr const char* strings[TPL_IMAGE_FORMAT_COUNT] = {
        "I4",      // TPL_IMAGE_FORMAT_I4
        "I8",      // TPL_IMAGE_FORMAT_I8
        "IA4",     // TPL_IMAGE_FORMAT_IA4
        "IA8",     // TPL_IMAGE_FORMAT_IA8
        "RGB565",  // TPL_IMAGE_FORMAT_RGB565
        "RGB5A3",  // TPL_IMAGE_FORMAT_RGB5A3
        "RGBA32",  // TPL_IMAGE_FORMAT_RGBA32
        "Invalid format", // Index 7 (unused)
        "C4",      // TPL_IMAGE_FORMAT_C4
        "C8",      // TPL_IMAGE_FORMAT_C8
        "C14X2",   // TPL_IMAGE_FORMAT_C14X2
        "Invalid format", // Index 11 (unused)
        "Invalid format", // Index 12 (unused)
        "Invalid format", // Index 13 (unused)
        "CMPR"     // TPL_IMAGE_FORMAT_CMPR
    };

    if (format >= TPL_IMAGE_FORMAT_COUNT)
        return "Invalid format";

    return strings[format];
}

constexpr bool getImageFormatCompressed(TPLImageFormat format) {
    return format == TPL_IMAGE_FORMAT_CMPR;
}

constexpr bool getImageFormatPaletted(TPLImageFormat format) {
    return
        format == TPL_IMAGE_FORMAT_C4 ||
        format == TPL_IMAGE_FORMAT_C8 ||
        format == TPL_IMAGE_FORMAT_C14X2;
}

struct TPLTexture {
public:
    unsigned width;
    unsigned height;

    unsigned mipCount;

    TPLWrapMode wrapS;
    TPLWrapMode wrapT;

    TPLTexFilter minFilter;
    TPLTexFilter magFilter;

    TPLImageFormat format;

    std::vector<unsigned char> data; // In RGBA32 format.
    std::vector<uint32_t> palette; // In RGBA32 format.

public:
    [[nodiscard]] GLuint createGPUTexture() const;
};

class TPLObject {
public:
    TPLObject(const unsigned char* tplData, const size_t dataSize);
    TPLObject() :
        mInitialized(true)
    {};

    bool isInitialized() const { return mInitialized; }

    [[nodiscard]] std::vector<unsigned char> Serialize();

public:
    bool mInitialized { false };

    std::vector<TPLTexture> mTextures;
};

} // namespace TPL

#endif // TPL_HPP
