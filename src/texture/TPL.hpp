#ifndef TPL_HPP
#define TPL_HPP

#include <cstdint>

#include <vector>

#include <string>

#include <optional>

#include "../_glInclude.hpp"

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

    // Color + Alpha (16-bit DXT1 compressed RGB565 with binary alpha)
    TPL_IMAGE_FORMAT_CMPR = 0x0E,

    TPL_IMAGE_FORMAT_COUNT
};

enum TPLClutFormat : uint32_t {
    // Grayscale + Alpha (8 bits for each)
    TPL_CLUT_FORMAT_IA8,

    // Color (5 bits for red, 6 bits for green, 5 bits for blue)
    TPL_CLUT_FORMAT_RGB565,

    // Color + Alpha (16-bit pixels)
    //   - Pixel type A: 5 bits for each color
    //   - Pixel type B: 4 bits for each color, 3 bit alpha
    TPL_CLUT_FORMAT_RGB5A3
};

const char* getImageFormatName(TPLImageFormat format);

struct TPLTexture {
    uint8_t mipMap;
    uint8_t _pad24[3];

    uint16_t width;
    uint16_t height;

    TPLWrapMode wrapS;
    TPLWrapMode wrapT;
    TPLTexFilter minFilter;
    TPLTexFilter magFilter;

    TPLImageFormat format;

    std::vector<unsigned char> data;
    std::vector<uint32_t> palette;
};

class TPLObject {
public:
    bool ok { false };

    std::vector<TPLTexture> textures;

    std::vector<unsigned char> Reserialize();

    TPLObject(const unsigned char* tplData, const size_t dataSize);

    TPLObject() {};
};

std::optional<TPLObject> readTPLFile(const std::string& filePath);

GLuint LoadTPLTextureIntoGLTexture(const TPL::TPLTexture& tplTexture);

} // namespace TPL

#endif // TPL_HPP
