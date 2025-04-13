#include "RvlPalette.hpp"

#include "../Logging.hpp"

#include "../common.hpp"

std::set<uint32_t> RvlPalette::generate(const unsigned char* _rgbaImage, unsigned pixelCount) {
    const uint32_t* rgbaImage = reinterpret_cast<const uint32_t*>(_rgbaImage);

    std::set<uint32_t> uniqueColors;
    for (unsigned i = 0; i < pixelCount; ++i)
        uniqueColors.insert(rgbaImage[i]);

    return uniqueColors;
}

void RvlPalette::readCLUT(
    std::vector<uint32_t>& colorsOut,
    const void* clutIn,

    const unsigned colorCount,

    const TPL::TPLClutFormat format
) {
    colorsOut.resize(colorCount);

    switch (format) {
    case TPL::TPL_CLUT_FORMAT_IA8: {
        const unsigned char* table = static_cast<const unsigned char*>(clutIn);

        for (unsigned i = 0; i < colorCount; i++) {
            const uint32_t alpha = table[(i * 2) + 0];
            const uint32_t intensity = table[(i * 2) + 1];

            colorsOut[i] = (intensity << 24) | (intensity << 16) | (intensity << 8) | alpha;
        }
    } break;
    case TPL::TPL_CLUT_FORMAT_RGB565: {
        const uint16_t* table = static_cast<const uint16_t*>(clutIn);

        for (unsigned i = 0; i < colorCount; i++) {
            const uint16_t sourcePixel = BYTESWAP_16(table[i]);

            const uint32_t r = ((sourcePixel >> 11) & 0x1f) << 3;
            const uint32_t g = ((sourcePixel >>  5) & 0x3f) << 2;
            const uint32_t b = ((sourcePixel >>  0) & 0x1f) << 3;

            colorsOut[i] = (r << 24) | (g << 16) | (b << 8) | 0xFFu;
        }
    } break;
    case TPL::TPL_CLUT_FORMAT_RGB5A3: {
        const uint16_t* table = static_cast<const uint16_t*>(clutIn);

        for (unsigned i = 0; i < colorCount; i++) {
            const uint16_t sourcePixel = BYTESWAP_16(table[i]);

            uint32_t r, g, b, a;

            if ((sourcePixel & (1 << 15)) != 0) { // RGB555
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

            colorsOut[i] = (r << 24) | (g << 16) | (b << 8) | a;
        }
    } break;

    default:
        Logging::err <<
            "[RvlPalette::readCLUT] Invalid color palette format! Expected 0 to " <<
            (TPL::TPL_CLUT_FORMAT_COUNT - 1) << ", got " << static_cast<int>(format) << '\n';
        return;
    }
}

bool RvlPalette::writeCLUT(
    void* clutOut,
    const std::vector<uint32_t>& colorsIn,

    const TPL::TPLClutFormat format
) {
    const unsigned colorCount = colorsIn.size();

    switch (format) {
    case TPL::TPL_CLUT_FORMAT_IA8: {
        Logging::err <<
            "[RvlPalette::writeCLUT] IA8 format not implemented!" << std::endl;
        return false;
    } break;
    case TPL::TPL_CLUT_FORMAT_RGB565: {
        Logging::err <<
            "[RvlPalette::writeCLUT] RGB565 format not implemented!" << std::endl;
        return false;
    } break;
    case TPL::TPL_CLUT_FORMAT_RGB5A3: {
        uint8_t* dest = static_cast<uint8_t*>(clutOut);

        for (unsigned i = 0; i < colorCount; i++) {
            const uint8_t* readPixel = reinterpret_cast<const uint8_t*>(&colorsIn[i]);

            // A pixel is 16-bit but we write it in two 8-bit segments.
            uint8_t* pixel = dest + (i * 2);

            // Opaque, write RGB555 pixel.
            if (readPixel[3] == 255) {
                // Bits:
                // 1    RRRRRGGGGG BBBBB
                // ^    ^    ^     ^
                // Type Red  Green Blue

                pixel[0] = 0x0080 | ((readPixel[0] & 0xF8) >> 1) | (readPixel[1] >> 6);
                pixel[1] = ((readPixel[1] & 0x38) << 2) | (readPixel[2] >> 3);
            }
            // Transparent, write RGBA4443 pixel.
            else {
                // Bits:
                // 0    AAA   RRRRGGGG  BBBB
                // ^    ^     ^   ^     ^
                // Type Alpha Red Green Blue

                pixel[0] = ((readPixel[3] >> 1) & 0x70) | ((readPixel[0] & 0xF0) >> 4);
                pixel[1] = (readPixel[1] & 0xF0) | ((readPixel[2] & 0xF0) >> 4);
            }
        }
    } break;

    default:
        Logging::err <<
            "[RvlPalette::WriteRGBA32Palette] Invalid color palette format! Expected 0 to " <<
            (TPL::TPL_CLUT_FORMAT_COUNT - 1) << ", got " << static_cast<int>(format) << std::endl;
        return false;
    }

    return true;
}
