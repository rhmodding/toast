#ifndef TPL_HPP
#define TPL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include <GL/gl.h>

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
        TPL_IMAGE_FORMAT_CMPR = 0x0E,

        TPL_IMAGE_FORMAT_COUNT
    };

    enum TPLClutFormat : uint32_t {
        TPL_CLUT_FORMAT_IA8, // Gray + Alpha
        TPL_CLUT_FORMAT_RGB565, // Color
        TPL_CLUT_FORMAT_RGB5A3 // Color + Alpha
    };

    const char* getImageFormatName(TPLImageFormat format);

    struct TPLTexture {
        uint8_t valid; // Did the texture have a valid offset?

        uint16_t width;
        uint16_t height;

        std::vector<unsigned char> data;

        std::vector<uint32_t> palette; 

        TPLWrapMode wrapS;
        TPLWrapMode wrapT;

        TPLTexFilter minFilter;
        TPLTexFilter magFilter;

        uint8_t mipMap;

        TPLImageFormat format; // This is used when re-serializing the TPL.
    };

    class TPLObject {
    public:
        std::vector<TPLTexture> textures;

        std::vector<unsigned char> Reserialize();

        TPLObject(const unsigned char* tplData, const size_t dataSize);

        TPLObject() {};
    };

    std::optional<TPLObject> readTPLFile(const std::string& filePath);

    GLuint LoadTPLTextureIntoGLTexture(TPL::TPLTexture tplTexture);
}

#endif // TPL_HPP