#include "ImageConvert.hpp"

#include <unordered_map>
#include <set>

#include <iostream>

#include "../common.hpp"

typedef TPL::TPLImageFormat ImageFormat;

typedef void (*FromImplementation)(unsigned char*, unsigned, unsigned, const unsigned char*, const uint32_t*);
typedef void (*ToImplementation)(unsigned char*, uint32_t*, unsigned*, unsigned, unsigned, const unsigned char*);

/*
    FROM implementations (x to RGBA32)
*/

static void IMPLEMENTATION_FROM_I4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 8) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 8; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x += 2) {
                    if (xx + x >= srcWidth) break;

                    const uint8_t intensityA = ((data[readOffset + (y * 4) + (x / 2)] & 0xF0) >> 4) * 0x11;
                    const uint8_t intensityB = (data[readOffset + (y * 4) + (x / 2)] & 0x0F) * 0x11;

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
            readOffset += 4 * 8;
        }
    }
}

static void IMPLEMENTATION_FROM_I8(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
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

static void IMPLEMENTATION_FROM_IA4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
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

static void IMPLEMENTATION_FROM_IA8(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
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


static void IMPLEMENTATION_FROM_RGB565(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
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

static void IMPLEMENTATION_FROM_RGB5A3(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
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

static void IMPLEMENTATION_FROM_RGBA32(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
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


static void IMPLEMENTATION_FROM_CMPR(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t*) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 8) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {
            // 4 4x4 RGBA blocks. Makes up one whole CMPR block
            uint8_t blocks[4][4][4][4];

            // Decode each CMPR-subblock
            for (unsigned i = 0; i < 4; i++) {
                const unsigned char* blockData = data + readOffset + (i * 8);
                uint8_t (*block)[4][4] = blocks[i];

                const uint16_t color1    = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(blockData + 0));
                const uint16_t color2    = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(blockData + 2));
                const uint32_t indexBits = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(blockData + 4));

                uint8_t colors[4][4];
                colors[0][0] = ((color1 >> 11) & 0x1f) << 3;
                colors[0][1] = ((color1 >> 5) & 0x3f) << 2;
                colors[0][2] = ((color1 >> 0) & 0x1f) << 3;
                colors[0][3] = 0xFFu;
                colors[1][0] = ((color2 >> 11) & 0x1f) << 3;
                colors[1][1] = ((color2 >> 5) & 0x3f) << 2;
                colors[1][2] = ((color2 >> 0) & 0x1f) << 3;
                colors[1][3] = 0xFFu;

                if (color1 > color2) {
                    for (unsigned j = 0; j < 4; ++j)
                        colors[2][j] = colors[0][j] * 2 / 3 + colors[1][j] / 3;
                    for (unsigned j = 0; j < 4; ++j)
                        colors[3][j] = colors[0][j] / 3 + colors[1][j] * 2 / 3;
                }
                else {
                    for (unsigned j = 0; j < 4; ++j)
                        colors[2][j] = (colors[0][j] + colors[1][j]) / 2;
                    colors[3][0] = colors[3][1] = colors[3][2] = colors[3][3] = 0x00;
                }

                uint8_t indices[16];
                for (unsigned j = 0; j < 16; j++)
                    indices[j] = (indexBits >> (j * 2)) & 0b11;

                for (unsigned y = 0; y < 4; y++) {
                    for (unsigned x = 0; x < 4; x++) {
                        unsigned index = 15 - ((y * 4) + x);

                        block[y][x][0] = colors[indices[index]][0];
                        block[y][x][1] = colors[indices[index]][1];
                        block[y][x][2] = colors[indices[index]][2];
                        block[y][x][3] = colors[indices[index]][3];
                    }
                }
            }

            // Copy decoded pixels
            for (unsigned y = 0; y < 8; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned writeOffset = (rowBase + xx + x) * 4;

                    unsigned blockIdx = (y >= 4) * 2 + (x >= 4);

                    const unsigned localY = y % 4;
                    const unsigned localX = x % 4;

                    result[writeOffset + 0] = blocks[blockIdx][localY][localX][0];
                    result[writeOffset + 1] = blocks[blockIdx][localY][localX][1];
                    result[writeOffset + 2] = blocks[blockIdx][localY][localX][2];
                    result[writeOffset + 3] = blocks[blockIdx][localY][localX][3];
                }
            }

            readOffset += 8 * 4; // Advance sizeof sub-block * sub-block count
        }
    }
}


static void IMPLEMENTATION_FROM_C8(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {    
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const uint32_t destIndex = (rowBase + xx + x) * 4;

                    const uint8_t index = data[readOffset + (y * 8) + x];
                    const uint32_t color = palette[index];

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

static void IMPLEMENTATION_FROM_C14X2(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data, const uint32_t* palette) {
    unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 4) {
        for (unsigned xx = 0; xx < srcWidth; xx += 4) {

            for (unsigned y = 0; y < 4; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned destIndex = (rowBase + xx + x) * 4;

                    const uint16_t index = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(
                        data + readOffset + (y * 2 * 4) + (x * 2)
                    ));
                    const uint32_t color = palette[index & 0x3FFF];

                    result[destIndex + 0] = (color >> 24) & 0xFFu;
                    result[destIndex + 1] = (color >> 16) & 0xFFu;
                    result[destIndex + 2] = (color >>  8) & 0xFFu;
                    result[destIndex + 3] = color & 0xFFu;
                }
            }
            readOffset += 2 * 4 * 4;

        }
    }
}

/*
    TO implementations (RGBA32 to x)
*/

static void IMPLEMENTATION_TO_RGB5A3(unsigned char* result, uint32_t*, unsigned*, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    unsigned writeOffset { 0 };

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

                    if (readPixel[3] == 0xFFu) { // Opaque, write RGB555 pixel
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

static void IMPLEMENTATION_TO_RGBA32(unsigned char* result, uint32_t*, unsigned*, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
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


static void IMPLEMENTATION_TO_C8(unsigned char* result, uint32_t* paletteOut, unsigned* paletteSizeOut, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
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

static void IMPLEMENTATION_TO_C14X2(unsigned char* result, uint32_t* paletteOut, unsigned* paletteSizeOut, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
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
    case ImageFormat::TPL_IMAGE_FORMAT_I4:
        implementation = IMPLEMENTATION_FROM_I4;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_I8:
        implementation = IMPLEMENTATION_FROM_I8;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_IA4:
        implementation = IMPLEMENTATION_FROM_IA4;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_IA8:
        implementation = IMPLEMENTATION_FROM_IA8;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_RGB565:
        implementation = IMPLEMENTATION_FROM_RGB565;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_RGB5A3:
        implementation = IMPLEMENTATION_FROM_RGB5A3;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_RGBA32:
        implementation = IMPLEMENTATION_FROM_RGBA32;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_C8:
        if (!palette) {
            std::cerr << "[ImageConvert::toRGBA32] Cannot convert C8 texture: palette is nullptr\n";
            return false;
        }

        implementation = IMPLEMENTATION_FROM_C8;
        break;
    case ImageFormat::TPL_IMAGE_FORMAT_C14X2:
        if (!palette) {
            std::cerr << "[ImageConvert::toRGBA32] Cannot convert C14X2 texture: palette is nullptr\n";
            return false;
        }

        implementation = IMPLEMENTATION_FROM_C14X2;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_CMPR:
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
    case ImageFormat::TPL_IMAGE_FORMAT_RGB5A3:
        implementation = IMPLEMENTATION_TO_RGB5A3;
        break;

    case ImageFormat::TPL_IMAGE_FORMAT_RGBA32:
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
    case ImageFormat::TPL_IMAGE_FORMAT_C14X2:
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
    if (texture.format == ImageFormat::TPL_IMAGE_FORMAT_C14X2)
        texture.palette.resize(16384);
    if (texture.format == ImageFormat::TPL_IMAGE_FORMAT_C8)
        texture.palette.resize(256);
    if (texture.format == ImageFormat::TPL_IMAGE_FORMAT_C4)
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
 	unsigned tilesX = (width + 7) / 8;
	unsigned tilesY = (height + 7) / 8;

	return tilesX * tilesY * 32;
}
static unsigned ImageByteSize_8(unsigned width, unsigned height) {
 	unsigned tilesX = (width + 7) / 8;
	unsigned tilesY = (height + 3) / 4;

	return tilesX * tilesY * 32;
}
static unsigned ImageByteSize_16(unsigned width, unsigned height) {
 	unsigned tilesX = (width + 3) / 4;
	unsigned tilesY = (height + 3) / 4;

	return tilesX * tilesY * 32;
}
static unsigned ImageByteSize_32(unsigned width, unsigned height) {
 	unsigned tilesX = (width + 3) / 4;
	unsigned tilesY = (height + 3) / 4;

	return tilesX * tilesY * 32 * 2;
}
static unsigned ImageByteSize_CMPR(unsigned width, unsigned height) {
 	unsigned tilesX = (width + 7) / 8;
	unsigned tilesY = (height + 7) / 8;

	return tilesX * tilesY * 32;
}

unsigned ImageConvert::getImageByteSize(const TPL::TPLImageFormat type, const unsigned width, const unsigned height) {
    switch (type) {
    case ImageFormat::TPL_IMAGE_FORMAT_I4:
        return ImageByteSize_4(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_I8:
        return ImageByteSize_8(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_IA4:
        return ImageByteSize_8(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_IA8:
        return ImageByteSize_16(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_RGB565:
        return ImageByteSize_16(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_RGB5A3:
        return ImageByteSize_16(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_RGBA32:
        return ImageByteSize_32(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_C4:
        return ImageByteSize_4(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_C8:
        return ImageByteSize_8(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_C14X2:
        return ImageByteSize_16(width, height);

    case ImageFormat::TPL_IMAGE_FORMAT_CMPR:
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
