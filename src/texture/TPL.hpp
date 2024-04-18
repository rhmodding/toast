#ifndef TPL_HPP
#define TPL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace TPL {
    enum TPLWrapMode {
        TPL_WRAP_MODE_CLAMP,
        TPL_WRAP_MODE_REPEAT,
        TPL_WRAP_MODE_MIRROR,
        TPL_WRAP_MODE_NONE
    };

    enum TPLTexFilter {
        TPL_TEX_FILTER_NEAR,
        TPL_TEX_FILTER_LINEAR,
        TPL_TEX_FILTER_NEAR_MIP_NEAR,
        TPL_TEX_FILTER_LIN_MIP_NEAR,
        TPL_TEX_FILTER_NEAR_MIP_LIN,
        TPL_TEX_FILTER_LIN_MIP_LIN
    };

    struct TPLTexture {
        uint16_t width;
        uint16_t height;

        std::vector<char> data;

        TPLWrapMode wrapS;
        TPLWrapMode wrapT;

        TPL::TPLTexFilter minFilter;
        TPL::TPLTexFilter magFilter;

        uint8_t mipMap;
    };

    enum TPLImageFormat : uint32_t {
        // Gray
        TPL_IMAGE_FORMAT_I4,

        // Gray
        TPL_IMAGE_FORMAT_I8,

        // Gray + Alpha
        TPL_IMAGE_FORMAT_IA4,

        // Gray + Alpha
        TPL_IMAGE_FORMAT_IA8,

        // Color
        TPL_IMAGE_FORMAT_RGB565,

        // Color + Alpha
        TPL_IMAGE_FORMAT_RGB5A3,

        // Color + Alpha
        TPL_IMAGE_FORMAT_RGBA32,

        // Palette (IA8, RGB565, RGB5A3)
        TPL_IMAGE_FORMAT_C4 = 0x08,

        // Palette (IA8, RGB565, RGB5A3)
        TPL_IMAGE_FORMAT_C8,

        // Palette (IA8, RGB565, RGB5A3)
        TPL_IMAGE_FORMAT_C14X2,

        // Color + optional Alpha (compressed)
        TPL_IMAGE_FORMAT_CMPR = 0x0E
    };

    class TPLObject {
    public:
        std::vector<TPLTexture> textures;

        TPLObject(const char* tplData, const size_t dataSize);
    };

    std::optional<TPLObject> readTPLFile(const std::string& filePath);
}

#endif // TPL_HPP