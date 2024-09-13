#ifndef PALETTEUTILS_HPP
#define PALETTEUTILS_HPP

#include "TPL.hpp"

#include <cstdint>

#include <vector>

#include "../common.hpp"

namespace PaletteUtils {

void CLUTtoRGBA32Palette(
    std::vector<uint32_t>& buffer,

    const uint8_t* data,
    const uint16_t numEntries,

    const TPL::TPLClutFormat format
) {
    buffer.resize(numEntries);

    switch (format) {
        case TPL::TPL_CLUT_FORMAT_IA8: {
            for (uint16_t i = 0; i < numEntries; i++) {
                const uint8_t alpha = data[(i * 2) + 0];
                const uint8_t intensity = data[(i * 2) + 1];

                buffer[i] =
                    (uint32_t(intensity) << 24) |
                    (uint32_t(intensity) << 16) |
                    (uint32_t(intensity) << 8) |
                    uint32_t(alpha);
            }
        } break;
        case TPL::TPL_CLUT_FORMAT_RGB565: {
            for (uint16_t i = 0; i < numEntries; i++) {
                const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data + (i * 2)));

                buffer[i] =
                    (uint32_t(((sourcePixel >> 11) & 0x1f) << 3) << 24) |
                    (uint32_t(((sourcePixel >>  5) & 0x3f) << 2) << 16) |
                    (uint32_t(((sourcePixel >>  0) & 0x1f) << 3) << 8) |
                    0xFFu;
            }
        } break;
        case TPL::TPL_CLUT_FORMAT_RGB5A3: {
            for (uint16_t i = 0; i < numEntries; i++) {
                const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data + (i * 2)));

                uint8_t r, g, b, a;

                if ((sourcePixel & 0x8000) != 0) { // RGB555
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

                buffer[i] =
                    (uint32_t(r) << 24) |
                    (uint32_t(g) << 16) |
                    (uint32_t(b) << 8) |
                    uint32_t(a);
            }
        } break;

        default:
            std::cerr <<
                "[PaletteUtils::toRGBA32] Invalid color palette format! Expected 0 to 2, got " <<
                format << '\n';
            return;
    }
}

bool WriteRGBA32Palette(
    const uint32_t* data,
    const uint16_t numEntries,

    const TPL::TPLClutFormat format,

    uint8_t* dest
) {
    switch (format) {
        case TPL::TPL_CLUT_FORMAT_IA8: {
            std::cerr <<
                "[PaletteUtils::WriteRGBA32Palette] IA8 format not implemented!\n";
            return false;
        } break;
        case TPL::TPL_CLUT_FORMAT_RGB565: {
            std::cerr <<
                "[PaletteUtils::WriteRGBA32Palette] RGB565 format not implemented!\n";
            return false;
        } break;
        case TPL::TPL_CLUT_FORMAT_RGB5A3: {
            for (uint16_t i = 0; i < numEntries; i++) {
                // A pixel is 16-bit but we write it in two 8-bit segments.
                uint8_t* pixel = reinterpret_cast<uint8_t*>(dest + (i * sizeof(uint16_t)));

                uint8_t r = (*(data + i) >> 24) & 0xFF;
                uint8_t g = (*(data + i) >> 16) & 0xFF;
                uint8_t b = (*(data + i) >>  8) & 0xFF;
                uint8_t a = (*(data + i) >>  0) & 0xFF;

                if (a == 255) { // Opaque, write RGB555 pixel
                    // Bits:
                    // 1    RRRRRGGGGG BBBBB
                    // ^    ^    ^     ^
                    // Type Red  Green Blue

                    *(pixel + 0) = 0x0080 | ((r & 0xF8) >> 1) | (g >> 6);
                    *(pixel + 1) = ((g & 0x38) << 2) | (b >> 3);
                }
                else { // Transparent, write RGBA4443
                    // Bits:
                    // 0    AAA   RRRRGGGG  BBBB
                    // ^    ^     ^   ^     ^
                    // Type Alpha Red Green Blue

                    *(pixel + 0) = ((a >> 1) & 0x70) | ((r & 0xF0) >> 4);
                    *(pixel + 1) = (g & 0xF0) | ((b & 0xF0) >> 4);
                }
            }
        } break;

        default:
            std::cerr <<
                "[PaletteUtils::WriteRGBA32Palette] Invalid color palette format! Expected 0 to 2, got " <<
                format << '\n';
            return false;
    }

    return true;
}

} // namespace PaletteUtils

#endif // PALETTEUTILS_HPP
