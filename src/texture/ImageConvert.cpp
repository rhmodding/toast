#include "ImageConvert.hpp"

#include <unordered_map>
#include <set>

#include <iostream>

#include "../common.hpp"

#define IMGFMT TPL::TPLImageFormat

/*                                 result          srcWidth  srcHeight data                  palette
**                                                                                          (not used by non-palette 
**                                                                                           implementations) */
typedef void (*FromImplementation)(unsigned char*, unsigned, unsigned, const unsigned char*, const uint32_t*);

/*                               result          paletteOut paletteSizeOut       srcHeight data
**                                                                     srcWidth
**                                               (not mutated by non-palette
**                                                implementations) */
typedef void (*ToImplementation)(unsigned char*, uint32_t*, unsigned*, unsigned, unsigned, const unsigned char*);

void RGB565ToRGBA32(uint16_t pixel, uint8_t* dest, uint32_t offset = 0) {
    uint8_t rU, gU, bU;

    rU = (pixel & 0xF100) >> 11;
    gU = (pixel & 0x7E0) >> 5;
    bU = (pixel & 0x1F);

    dest[offset] = (rU << (8 - 5)) | (rU >> (10 - 8));
    dest[offset + 1] = (gU << (8 - 6)) | (gU >> (12 - 8));
    dest[offset + 2] = (bU << (8 - 5)) | (bU >> (10 - 8));
    dest[offset + 3] = 0xFFu;
}

//////////////////////////////////////////////////////////////////

#pragma region FROM IMPLEMENTATIONS

#pragma region Ixx

void IMPLEMENTATION_FROM_I4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 8) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 8; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x += 2) {
                    if (xx + x >= srcWidth) break;

                    const uint8_t intensityA = ((data[readOffset + (y * 8) + x] & 0xF0) >> 4) * 0x11;
                    const uint8_t intensityB = (data[readOffset + (y * 8) + x] & 0x0F) * 0x11;

                    const unsigned destIndex = (rowBase + xx + x) * 4;

                    result[destIndex + 0] = intensityA;
                    result[destIndex + 1] = intensityA;
                    result[destIndex + 2] = intensityA;
                    result[destIndex + 3] = 0xFFu;

                    result[destIndex + 4] = intensityB;
                    result[destIndex + 5] = intensityB;
                    result[destIndex + 6] = intensityB;
                    result[destIndex + 7] = 0xFFu;
                }
            }
            readOffset += 1 * 8 * 8;
        }
    }
}

void IMPLEMENTATION_FROM_I8(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const uint8_t intensity = data[readOffset + (y * 8) + x];

                    const uint32_t destIndex = 4 * (srcWidth * (yy + y) + xx + x);

                    result[destIndex + 0] = intensity;
                    result[destIndex + 1] = intensity;
                    result[destIndex + 2] = intensity;
                    result[destIndex + 3] = 0xFFu;
                }
            }
            readOffset += 1 * 8 * 4;
        }
    }
}

void IMPLEMENTATION_FROM_IA4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned destIndex = (rowBase + xx + x) * 4;

                    const uint8_t alpha = ((data[readOffset + (y * 8) + x] & 0xF0) >> 4) * 0x11;
                    const uint8_t intensity = (data[readOffset + (y * 8) + x] & 0x0F) * 0x11;

                    result[destIndex + 0] = intensity;
                    result[destIndex + 1] = intensity;
                    result[destIndex + 2] = intensity;
                    result[destIndex + 3] = alpha;
                }
            }
            readOffset += 1 * 8 * 4;
        }
    }
}

void IMPLEMENTATION_FROM_IA8(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned destIndex = (rowBase + xx + x) * 4;

                    const uint8_t alpha = data[readOffset + (y * 2 * 4) + (x * 2) + 0];
                    const uint8_t intensity = data[readOffset + (y * 2 * 4) + (x * 2) + 1];

                    result[destIndex + 0] = intensity;
                    result[destIndex + 1] = intensity;
                    result[destIndex + 2] = intensity;
                    result[destIndex + 3] = alpha;
                }
            }
            readOffset += 2 * 4 * 4;
        }
    }
}

#pragma endregion

#pragma region RGBxxx

void IMPLEMENTATION_FROM_RGB565(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned writeOffset = (rowBase + xx + x) * 4;

                    const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(
                        data + readOffset + (y * 2 * 4) + (x * 2)
                    ));

                    result[writeOffset + 0] = ((sourcePixel >> 11) & 0x1f) << 3;
                    result[writeOffset + 1] = ((sourcePixel >>  5) & 0x3f) << 2;
                    result[writeOffset + 2] = ((sourcePixel >>  0) & 0x1f) << 3;
                    result[writeOffset + 3] = 0xFFu;
                }
            }
            readOffset += 2 * 4 * 4;
        }
    }
}

void IMPLEMENTATION_FROM_RGB5A3(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned writeOffset = (rowBase + xx + x) * 4;

                    const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(
                        data + readOffset + (y * 2 * 4) + (x * 2)
                    ));

                    if ((sourcePixel & 0x8000) != 0) { // RGB555
                        result[writeOffset + 0] = (((sourcePixel >> 10) & 0x1f) << 3) | (((sourcePixel >> 10) & 0x1f) >> 2);
                        result[writeOffset + 1] = (((sourcePixel >> 5) & 0x1f) << 3) | (((sourcePixel >> 5) & 0x1f) >> 2);
                        result[writeOffset + 2] = (((sourcePixel) & 0x1f) << 3) | (((sourcePixel) & 0x1f) >> 2);

                        result[writeOffset + 3] = 0xFFu;
                    }
                    else { // RGBA4443
                        result[writeOffset + 0] = (((sourcePixel >> 8) & 0x0f) << 4) | ((sourcePixel >> 8) & 0x0f);
                        result[writeOffset + 1] = (((sourcePixel >> 4) & 0x0f) << 4) | ((sourcePixel >> 4) & 0x0f);
                        result[writeOffset + 2] = (((sourcePixel) & 0x0f) << 4) | ((sourcePixel) & 0x0f);

                        result[writeOffset + 3] =
                            (((sourcePixel >> 12) & 0x07) << 5) | (((sourcePixel >> 12) & 0x07) << 2) |
                            (((sourcePixel >> 12) & 0x07) >> 1);
                    }
                }
            }
            readOffset += 2 * 4 * 4;

        }
    }
}

void IMPLEMENTATION_FROM_RGBA32(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {
            // The block data is split down into two subblocks:
            //    Subblock A: Alpha and Red channel
            //    Subblock B: Green and Blue channel

            // Subblock A
            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned destIndex = (rowBase + xx + x) * 4;

                    result[destIndex + 3] = data[readOffset + (y * 2 * 4) + (x * 2) + 0]; // Alpha channel
                    result[destIndex + 0] = data[readOffset + (y * 2 * 4) + (x * 2) + 1]; // Red channel
                }
            }
            readOffset += 2 * 4 * 4;

            // Subblock B
            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned destIndex = (rowBase + xx + x) * 4;

                    result[destIndex + 1] = data[readOffset + (y * 2 * 4) + (x * 2) + 0]; // Green channel
                    result[destIndex + 2] = data[readOffset + (y * 2 * 4) + (x * 2) + 1]; // Blue channel
                }
            }
            readOffset += 2 * 4 * 4;
        }
    }
}

#pragma endregion

#pragma region Cx

void IMPLEMENTATION_FROM_C8(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {    
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const uint8_t index = data[readOffset + (y * 8) + x];
                    const uint32_t color = palette[index];

                    const uint32_t destIndex = 4 * (srcWidth * (yy + y) + xx + x);

                    result[destIndex + 0] = (color >> 24) & 0xFFu;
                    result[destIndex + 1] = (color >> 16) & 0xFFu;
                    result[destIndex + 2] = (color >>  8) & 0xFFu;
                    result[destIndex + 3] = color & 0xFFu;
                }
            }
            readOffset += 1 * 8 * 4;
        }
    }
}

#pragma endregion

#pragma region CMPR

void IMPLEMENTATION_FROM_CMPR(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    // TODO: fix CMPR

    for (unsigned y = 0; y < srcHeight; y += 4) {
        for (unsigned x = 0; x < srcWidth; x += 4) {
            const uint16_t color1 = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data + readOffset));
            readOffset += sizeof(uint16_t);

            const uint16_t color2 = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data + readOffset));
            readOffset += sizeof(uint16_t);

            uint32_t bits = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(data + readOffset));
            readOffset += sizeof(uint32_t);

            uint8_t table[4][4] = {};

            RGB565ToRGBA32(color1, table[0]);
            RGB565ToRGBA32(color2, table[1]);

            if (color1 > color2) {
                table[2][0] = (2 * table[0][0] + table[1][0] + 1) / 3;
                table[2][1] = (2 * table[0][1] + table[1][1] + 1) / 3;
                table[2][2] = (2 * table[0][2] + table[1][2] + 1) / 3;
                table[2][3] = 0xFFu;

                table[3][0] = (table[0][0] + 2 * table[1][0] + 1) / 3;
                table[3][1] = (table[0][1] + 2 * table[1][1] + 1) / 3;
                table[3][2] = (table[0][2] + 2 * table[1][2] + 1) / 3;
                table[3][3] = 0xFFu;
            }
            else {
                table[2][0] = (table[0][0] + table[1][0] + 1) / 2;
                table[2][1] = (table[0][1] + table[1][1] + 1) / 2;
                table[2][2] = (table[0][2] + table[1][2] + 1) / 2;
                table[2][3] = 0xFFu;

                table[3][0] = (table[0][0] + 2 * table[1][0] + 1) / 3;
                table[3][1] = (table[0][1] + 2 * table[1][1] + 1) / 3;
                table[3][2] = (table[0][2] + 2 * table[1][2] + 1) / 3;
                table[3][3] = 0x00u;
            }

            for (unsigned iy = 0; iy < 4; ++iy) {
                for (unsigned ix = 0; ix < 4; ++ix) {
                    if (((x + ix) < srcWidth) && ((y + iy) < srcHeight)) {
                        uint32_t di = 4 * ((y + iy) * srcWidth + x + ix);
                        uint32_t si = bits & 0x3;

                        result[di + 0] = table[si][0];
                        result[di + 1] = table[si][1];
                        result[di + 2] = table[si][2];
                        result[di + 3] = table[si][3];
                    }

                    bits >>= 2;
                }
            }
        }
    }
}

#pragma endregion

#pragma endregion

//////////////////////////////////////////////////////////////////

#pragma region TO IMPLEMENTATIONS

#pragma region RGBxxx

void IMPLEMENTATION_TO_RGB5A3(unsigned char* result, uint32_t* paletteOut, unsigned* paletteSizeOut, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    unsigned writeOffset{ 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const uint8_t* readPixel = data + (rowBase + xx + x) * 4;

                    // A pixel is 16-bit but we write it in two 8-bit segments.
                    uint8_t* pixel = result + writeOffset + (y * 2 * 4) + (x * 2);

                    if (readPixel[3] == 255) { // Opaque, write RGB555 pixel
                        // Bits:
                        // 1    RRRRRGGGGG BBBBB
                        // ^    ^    ^     ^
                        // Type Red  Green Blue

                        pixel[0] = 0x0080 | ((readPixel[0] & 0xF8) >> 1) | (readPixel[1] >> 6);
                        pixel[1] = ((readPixel[1] & 0x38) << 2) | (readPixel[2] >> 3);
                    }
                    else { // Transparent, write RGBA4443
                        // Bits:
                        // 0    AAA   RRRRGGGG  BBBB
                        // ^    ^     ^   ^     ^
                        // Type Alpha Red Green Blue

                        pixel[0] = ((readPixel[3] >> 1) & 0x70) | ((readPixel[0] & 0xF0) >> 4);
                        pixel[1] = (readPixel[1] & 0xF0) | ((readPixel[2] & 0xF0) >> 4);
                    }
                }
            }
            writeOffset += 2 * 4 * 4;
        }
    }
}

void IMPLEMENTATION_TO_RGBA32(unsigned char* result, uint32_t* paletteOut, unsigned* paletteSizeOut, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    unsigned writeOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {
            // The block data is split down into two subblocks:
            //    Subblock A: Alpha and Red channel
            //    Subblock B: Green and Blue channel

            // Subblock A
            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned readOffset = (rowBase + xx + x) * 4;

                    result[writeOffset + (y * 2 * 4) + (x * 2) + 0] = data[readOffset + 3]; // Alpha channel
                    result[writeOffset + (y * 2 * 4) + (x * 2) + 1] = data[readOffset + 0]; // Red channel
                }
            }
            writeOffset += 2 * 4 * 4;

            // Subblock B
            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned readOffset = (rowBase + xx + x) * 4;

                    result[writeOffset + (y * 2 * 4) + (x * 2) + 0] = data[readOffset + 1]; // Green channel
                    result[writeOffset + (y * 2 * 4) + (x * 2) + 1] = data[readOffset + 2]; // Blue channel
                }
            }
            writeOffset += 2 * 4 * 4;
        }
    }
}

#pragma endregion

#pragma region Cx

void IMPLEMENTATION_TO_C8(unsigned char* result, uint32_t* paletteOut, unsigned* paletteSizeOut, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    std::unordered_map<uint32_t, uint8_t> colorToIndex;
    unsigned nextColorIndex { 0 };

    unsigned writeOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {
            
            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const uint32_t readPixel = *reinterpret_cast<const uint32_t*>(
                        data + (rowBase + xx + x) * 4
                    );
                    uint8_t* pixel = reinterpret_cast<uint8_t*>(
                        result + writeOffset + (y * 8) + x
                    );

                    if (colorToIndex.find(readPixel) == colorToIndex.end()) {
                        if (nextColorIndex >= 256) {
                            std::cerr << "[IMPLEMENTATION_TO_C8] Palette index limit reached (256), processing cannot continue.\n";
                            return;
                        }

                        paletteOut[nextColorIndex] = readPixel;
                        colorToIndex[readPixel] = nextColorIndex++;
                    }
                    *pixel = colorToIndex[readPixel];
                }
            }
            writeOffset += 1 * 8 * 4;
        }
    }

    *paletteSizeOut = nextColorIndex;
}

void IMPLEMENTATION_TO_C14X2(unsigned char* result, uint32_t* paletteOut, unsigned* paletteSizeOut, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    std::unordered_map<uint32_t, uint8_t> colorToIndex;
    unsigned nextColorIndex { 0 };

    unsigned writeOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const uint32_t readPixel = *reinterpret_cast<const uint32_t*>(
                        data + (rowBase + xx + x) * 4
                    );
                    uint16_t* pixel = reinterpret_cast<uint16_t*>(
                        result + writeOffset + (y * 2 * 4) + (x * 2)
                    );

                    if (colorToIndex.find(readPixel) == colorToIndex.end()) {
                        if (nextColorIndex >= 16384) {
                            std::cerr << "[IMPLEMENTATION_TO_C14X2] Palette index limit reached (16384), processing cannot continue.\n";
                            return;
                        }

                        paletteOut[nextColorIndex] = readPixel;
                        colorToIndex[readPixel] = nextColorIndex++;
                    }
                    *pixel = BYTESWAP_16(colorToIndex[readPixel]);
                }
            }
            writeOffset += 2 * 4 * 4;
        }
    }

    *paletteSizeOut = nextColorIndex;
}

#pragma endregion

#pragma endregion

//////////////////////////////////////////////////////////////////

bool ImageConvert::toRGBA32(
    unsigned char* buffer,
    const TPL::TPLImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data,
    const uint32_t* palette
) {
    FromImplementation implementation;
    switch (format) {
        case IMGFMT::TPL_IMAGE_FORMAT_I4:
            implementation = IMPLEMENTATION_FROM_I4;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_I8:
            implementation = IMPLEMENTATION_FROM_I8;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_IA4:
            implementation = IMPLEMENTATION_FROM_IA4;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_IA8:
            implementation = IMPLEMENTATION_FROM_IA8;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB565:
            implementation = IMPLEMENTATION_FROM_RGB565;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            implementation = IMPLEMENTATION_FROM_RGB5A3;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            implementation = IMPLEMENTATION_FROM_RGBA32;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            if (!palette) {
                std::cerr << "[ImageConvert::toRGBA32] Cannot convert C8 texture: palette is nullptr\n";
                return false;
            }

            implementation = IMPLEMENTATION_FROM_C8;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_CMPR:
            implementation = IMPLEMENTATION_FROM_CMPR;
            break;

        default:
            std::cerr << "[ImageConvert::toRGBA32] Cannot convert texture: invalid format (" << format << ")\n";
            return false;
    }

    implementation(buffer, srcWidth, srcHeight, data, palette);

    return true;
}

bool ImageConvert::toRGBA32(
    TPL::TPLTexture& texture,
    const unsigned char* data
) {
    return ImageConvert::toRGBA32(
        texture.data.data(),
        texture.format,
        texture.width, texture.height,
        data,
        texture.palette.data()
    );
}

bool ImageConvert::fromRGBA32(
    unsigned char* buffer,
    uint32_t* paletteOut,
    unsigned* paletteSizeOut,
    const TPL::TPLImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data
) {
    ToImplementation implementation;
    switch (format) {
        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            implementation = IMPLEMENTATION_TO_RGB5A3;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            implementation = IMPLEMENTATION_TO_RGBA32;
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            if (!paletteOut) {
                std::cerr << "[ImageConvert::fromRGBA32] Couldn't convert to C8 format: no color palette passed.\n";
                return false;
            }

            //IMPLEMENTATION_TO_C8(buffer, paletteOut, paletteSizeOut, srcWidth, srcHeight, data);
            implementation = IMPLEMENTATION_TO_C8;
            break;
        case IMGFMT::TPL_IMAGE_FORMAT_C14X2:
            if (!paletteOut) {
                std::cerr << "[ImageConvert::fromRGBA32] Couldn't convert to C14X2 format: no color palette passed.\n";
                return false;
            }

            implementation = IMPLEMENTATION_TO_C14X2;
            break;

        default:
            return false;
    }

    implementation(buffer, paletteOut, paletteSizeOut, srcWidth, srcHeight, data);

    return true;
}
bool ImageConvert::fromRGBA32(
    TPL::TPLTexture& texture,
    unsigned char* buffer
) {
    if (texture.format == IMGFMT::TPL_IMAGE_FORMAT_C14X2)
        texture.palette.resize(16384);
    if (texture.format == IMGFMT::TPL_IMAGE_FORMAT_C8)
        texture.palette.resize(256);
    if (texture.format == IMGFMT::TPL_IMAGE_FORMAT_C4)
        texture.palette.resize(16);

    unsigned paletteSize { 0 };

    bool result = fromRGBA32(
        buffer,
        texture.palette.data(),
        &paletteSize,
        texture.format,
        texture.width, texture.height,
        texture.data.data()
    );

    texture.palette.resize(paletteSize);

    return result;
}

static unsigned ImageByteSize_4(unsigned width, unsigned height) {
 	unsigned tilesX = ((width + 7) / 8);
	unsigned tilesY = ((height + 7) / 8);

	return tilesX * tilesY * 32;
}
static unsigned ImageByteSize_8(unsigned width, unsigned height) {
 	unsigned tilesX = ((width + 7) / 8);
	unsigned tilesY = ((height + 3) / 4);

	return tilesX * tilesY * 32;
}
static unsigned ImageByteSize_16(unsigned width, unsigned height) {
 	unsigned tilesX = ((width + 3) / 4);
	unsigned tilesY = ((height + 3) / 4);

	return tilesX * tilesY * 32;
}
static unsigned ImageByteSize_32(unsigned width, unsigned height) {
 	unsigned tilesX = ((width + 3) / 4);
	unsigned tilesY = ((height + 3) / 4);

	return tilesX * tilesY * 32 * 2;
}
static unsigned ImageByteSize_CMPR(unsigned width, unsigned height) {
 	unsigned tilesX = ((width + 7) / 8);
	unsigned tilesY = ((height + 7) / 8);

	return tilesX * tilesY * 32;
}

unsigned ImageConvert::getImageByteSize(const TPL::TPLImageFormat type, const unsigned width, const unsigned height) {
    switch (type) {
        case IMGFMT::TPL_IMAGE_FORMAT_I4:
            return ImageByteSize_4(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_I8:
            return ImageByteSize_8(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_IA4:
            return ImageByteSize_8(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_IA8:
            return ImageByteSize_16(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_RGB565:
            return ImageByteSize_16(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            return ImageByteSize_16(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            return ImageByteSize_32(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_C4:
            return ImageByteSize_4(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            return ImageByteSize_8(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_C14X2:
            return ImageByteSize_16(width, height);

        case IMGFMT::TPL_IMAGE_FORMAT_CMPR:
            return ImageByteSize_CMPR(width, height);

        default:
            std::cerr << "[ImageConvert::getImageByteSize] Invalid format passed (" << (int)type << ")\n";
            return 0;
    }

    // NOT REACHED
}

unsigned ImageConvert::getImageByteSize(const TPL::TPLTexture& texture) {
    return getImageByteSize(texture.format, texture.width, texture.height);
}
