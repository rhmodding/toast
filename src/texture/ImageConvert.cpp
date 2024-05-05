#include "ImageConvert.hpp"

#include "TPL.hpp"

#include "../common.hpp"

#define IMGFMT TPL::TPLImageFormat

#define RGBA32_PIXEL_SIZE 4

void RGB565ToRGBA32(uint16_t pixel, uint8_t* dest, int offset = 0) {
    uint8_t r, g, b;
    r = (pixel & 0xF100) >> 11;
    g = (pixel & 0x7E0) >> 5;
    b = (pixel & 0x1F);

    r = (r << (8 - 5)) | (r >> (10 - 8));
    g = (g << (8 - 6)) | (g >> (12 - 8));
    b = (b << (8 - 5)) | (b >> (10 - 8));

    dest[offset] = r;
    dest[offset + 1] = g;
    dest[offset + 2] = b;
    dest[offset + 3] = 0xFFu;
}

//////////////////////////////////////////////////////////////////

#pragma region FROM IMPLEMENTATIONS

#pragma region Ixx

void IMPLEMENTATION_FROM_I4(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    size_t readOffset = 0;

    int numBlocksX = srcWidth / 8; // 8 byte block width
    int numBlocksY = srcHeight / 8;

    for (int yBlock = 0; yBlock < numBlocksY; yBlock++) {
        for (int xBlock = 0; xBlock < numBlocksX; xBlock++) {

            for (int pY = 0; pY < 8; pY++) {
                for (int pX = 0; pX < 8; pX += 2) {
                    // Ensure the pixel we're checking is within bounds of the image.
                    if ((xBlock * 8 + pX >= srcWidth) || (yBlock * 8 + pY >= srcHeight))
                        continue;

                    char byte = data[readOffset];
                    readOffset++;

                    char t = (byte & 0xF0) >> 4;
                    char t2 = byte & 0x0F;

                    int destIndex = 4 * (srcWidth * ((yBlock * 8) + pY) + (xBlock * 8) + pX);

                    result[destIndex + 0] = t * 0x11;
                    result[destIndex + 1] = t * 0x11;
                    result[destIndex + 2] = t * 0x11;
                    result[destIndex + 3] = t * 0x11;

                    result[destIndex + 4] = t2 * 0x11;
                    result[destIndex + 5] = t2 * 0x11;
                    result[destIndex + 6] = t2 * 0x11;
                    result[destIndex + 7] = t2 * 0x11;
                }
            }

        }
    }
}

void IMPLEMENTATION_FROM_I8(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    for (int y = 0; y < srcHeight; ++y) {
        for (int x = 0; x < srcWidth; ++x) {
            // Index of the current pixel in the I8 texture
            int i8Index = y * srcWidth + x;

            // Index of the current pixel in the RGBA32 texture
            int rgba32Index = (y * srcWidth + x) * RGBA32_PIXEL_SIZE;

            // Extract the 8-bit intensity value from the I8 texture
            char intensity8Bits = data[i8Index];

            // Set the RGBA32 values
            result[rgba32Index] = intensity8Bits;           // Red
            result[rgba32Index + 1] = intensity8Bits;       // Green
            result[rgba32Index + 2] = intensity8Bits;       // Blue
            result[rgba32Index + 3] = 0xFFu;                // Alpha (fully opaque)
        }
    }
}

void IMPLEMENTATION_FROM_IA4(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    size_t readOffset = 0;

    int numBlocksX = srcWidth / 8;
    int numBlocksY = srcHeight / 4;

    for (int yBlock = 0; yBlock < numBlocksY; yBlock++) {
        for (int xBlock = 0; xBlock < numBlocksX; xBlock++) {

            for (int pY = 0; pY < 4; pY++) {
                for (int pX = 0; pX < 8; pX++) {
                    // Ensure the pixel we're checking is within bounds of the image.
                    if ((xBlock * 8 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    char byte = data[readOffset];
                    readOffset++;

                    char alpha = (byte & 0xF0) >> 4;
                    char luminosity = byte & 0x0F;

                    int destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 8) + pX);

                    result[destIndex + 0] = luminosity * 0x11;
                    result[destIndex + 1] = luminosity * 0x11;
                    result[destIndex + 2] = luminosity * 0x11;
                    result[destIndex + 3] = alpha * 0x11;
                }
            }

        }
    }
}

void IMPLEMENTATION_FROM_IA8(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    for (int y = 0; y < srcHeight; ++y) {
        for (int x = 0; x < srcWidth; ++x) {
            // Index of the current pixel in the IA8 texture
            int ia8Index = y * srcWidth + x;

            // Index of the current pixel in the RGBA32 texture
            int rgba32Index = (y * srcWidth + x) * RGBA32_PIXEL_SIZE;

            // Extract the 8-bit intensity and 8-bit alpha values from the IA8 texture
            char intensity8Bits = data[ia8Index];
            char alpha8Bits = data[ia8Index + 1];

            // Set the RGBA32 values
            result[rgba32Index] = intensity8Bits;           // Red
            result[rgba32Index + 1] = intensity8Bits;       // Green
            result[rgba32Index + 2] = intensity8Bits;       // Blue
            result[rgba32Index + 3] = alpha8Bits;           // Alpha
        }
    }
}

#pragma endregion

#pragma region RGBxxx

void IMPLEMENTATION_FROM_RGB565(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    for (int y = 0; y < srcHeight; ++y) {
        for (int x = 0; x < srcWidth; ++x) {
            // Index of the current pixel in the RGB565 texture
            int rgb565Index = (y * srcWidth + x) * sizeof(short);

            // Index of the current pixel in the RGBA32 texture
            int rgba32Index = (y * srcWidth + x) * RGBA32_PIXEL_SIZE;

            // Extract the RGB565 values
            short rgb565Value;
            memcpy(&rgb565Value, &data[rgb565Index], sizeof(short));

            // Convert RGB565 to RGBA32
            char red8Bits = (rgb565Value & 0xF800) >> 8;
            char green8Bits = (rgb565Value & 0x07E0) >> 3;
            char blue8Bits = (rgb565Value & 0x001F) << 3;

            // Set the RGBA32 values
            result[rgba32Index] = red8Bits;       // Red
            result[rgba32Index + 1] = green8Bits; // Green
            result[rgba32Index + 2] = blue8Bits;  // Blue
            result[rgba32Index + 3] = 0xFFu;        // Alpha (fully opaque)
        }
    }
}

void IMPLEMENTATION_FROM_RGB5A3(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    size_t readOffset = 0;

    for (int yy = 0; yy < srcHeight; yy += 4) {
        for (int xx = 0; xx < srcWidth; xx += 4) {              
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth || yy + y >= srcHeight)
                        continue;

                    int destPixel = (srcWidth * (yy + y)) + xx + x;
                    size_t writeOffset = destPixel * 4;

                    const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data.data() + readOffset));
                    readOffset += sizeof(uint16_t);

                    uint8_t r, g, b, a;

                    if ((sourcePixel & 0x8000) != 0) { // RGB555
                        r = (((sourcePixel >> 10) & 0x1f) << 3) | (((sourcePixel >> 10) & 0x1f) >> 2);
                        g = (((sourcePixel >> 5) & 0x1f) << 3) | (((sourcePixel >> 5) & 0x1f) >> 2);
                        b = (((sourcePixel) & 0x1f) << 3) | (((sourcePixel) & 0x1f) >> 2);

                        a = 0xFFu;
                    }
                    else { // RGBA4443
                        a = (((sourcePixel >> 12) & 0x7) << 5) | (((sourcePixel >> 12) & 0x7) << 2) |
                            (((sourcePixel >> 12) & 0x7) >> 1);

                        r = (((sourcePixel >> 8) & 0xf) << 4) | ((sourcePixel >> 8) & 0xf);
                        g = (((sourcePixel >> 4) & 0xf) << 4) | ((sourcePixel >> 4) & 0xf);
                        b = (((sourcePixel) & 0xf) << 4) | ((sourcePixel) & 0xf);
                    }

                    result[writeOffset + 0] = r;
                    result[writeOffset + 1] = g;
                    result[writeOffset + 2] = b;
                    result[writeOffset + 3] = a;
                }
            }
        }
    }
}

void IMPLEMENTATION_FROM_RGBA32(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    size_t readOffset = 0;

    int numBlocksW = srcWidth / 4;
    int numBlocksH = srcHeight / 4;

    for (int yBlock = 0; yBlock < numBlocksH; yBlock++) {
        for (int xBlock = 0; xBlock < numBlocksW; xBlock++) {

            // 1st subblock
            for (int pY = 0; pY < 4; pY++) {
                for (int pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    int destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);
                    result[destIndex + 3] = data[readOffset + 0]; // alpha
                    result[destIndex + 0] = data[readOffset + 1]; // red

                    readOffset += 2;
                }
            }

            // 2nd subblock
            for (int pY = 0; pY < 4; pY++) {
                for (int pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    int destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);
                    result[destIndex + 1] = data[readOffset + 0]; // green
                    result[destIndex + 2] = data[readOffset + 1]; // blue

                    readOffset += 2;
                }
            }

        }
    }
}

#pragma endregion

#pragma region CMPR // Open this if you love Pain

void IMPLEMENTATION_FROM_CMPR(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    size_t readOffset = 0;

    for (int y = 0; y < srcHeight; y += 4) {
        for (int x = 0; x < srcWidth; x += 4) {
            const uint16_t color1 = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data.data() + readOffset));
            readOffset += sizeof(uint16_t);

            const uint16_t color2 = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data.data() + readOffset));
            readOffset += sizeof(uint16_t);

            uint32_t bits = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(data.data() + readOffset));
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

            for (int iy = 0; iy < 4; ++iy) {
                for (int ix = 0; ix < 4; ++ix) {
                    if (((x + ix) < srcWidth) && ((y + iy) < srcHeight)) {
                        int di = 4 * ((y + iy) * srcWidth + x + ix);
                        int si = bits & 0x3;

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

void IMPLEMENTATION_TO_RGBA32(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    size_t writeOffset = 0;

    int numBlocksW = srcWidth / 4;
    int numBlocksH = srcHeight / 4;

    for (int yBlock = 0; yBlock < numBlocksH; yBlock++) {
        for (int xBlock = 0; xBlock < numBlocksW; xBlock++) {
            
            // 1st subblock
            for (int pY = 0; pY < 4; pY++) {
                for (int pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    int fromIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);
                    result[writeOffset + 0] = data[fromIndex + 3]; // alpha
                    result[writeOffset + 1] = data[fromIndex + 0]; // red

                    writeOffset += 2;
                }
            }

            // 2nd subblock
            for (int pY = 0; pY < 4; pY++) {
                for (int pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    int fromIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);
                    result[writeOffset + 0] = data[fromIndex + 1]; // green
                    result[writeOffset + 1] = data[fromIndex + 2]; // blue

                    writeOffset += 2;
                }
            }

        }
    }
}

#pragma endregion

#pragma endregion

//////////////////////////////////////////////////////////////////

bool ImageConvert::toRGBA32(std::vector<char>& result, const TPL::TPLImageFormat type, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    result.resize(srcWidth * srcHeight * RGBA32_PIXEL_SIZE);

    switch (type) {
        case IMGFMT::TPL_IMAGE_FORMAT_I4: 
            IMPLEMENTATION_FROM_I4(result, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_I8:
            IMPLEMENTATION_FROM_I8(result, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_IA4:
            IMPLEMENTATION_FROM_IA4(result, srcWidth, srcHeight, data);
            break;
        
        case IMGFMT::TPL_IMAGE_FORMAT_IA8:
            IMPLEMENTATION_FROM_IA8(result, srcWidth, srcHeight, data);
            break;
        
        case IMGFMT::TPL_IMAGE_FORMAT_RGB565:
            IMPLEMENTATION_FROM_RGB565(result, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            IMPLEMENTATION_FROM_RGB5A3(result, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            IMPLEMENTATION_FROM_RGBA32(result, srcWidth, srcHeight, data);
            break;

        // TODO: Palette-based formats

        case IMGFMT::TPL_IMAGE_FORMAT_CMPR:
            IMPLEMENTATION_FROM_CMPR(result, srcWidth, srcHeight, data);
            break;

        default:
            return false;
    }

    return true;
}

bool ImageConvert::fromRGBA32(std::vector<char>& result, const TPL::TPLImageFormat type, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    switch (type) {
        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            IMPLEMENTATION_TO_RGBA32(result, srcWidth, srcHeight, data);
            break;
        
        default:
            return false;
    }
    
    return true;
}

size_t ImageConvert::getImageByteSize(const TPL::TPLImageFormat type, uint16_t width, uint16_t height) {
    switch (type) {
        case IMGFMT::TPL_IMAGE_FORMAT_I4:
            return width * height / 2;

        case IMGFMT::TPL_IMAGE_FORMAT_I8:
            return width * height;

        case IMGFMT::TPL_IMAGE_FORMAT_IA4:
            return width * height;

        case IMGFMT::TPL_IMAGE_FORMAT_IA8:
            return width * height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_C4:
            return width * height / 2;

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            return width * height;

        case IMGFMT::TPL_IMAGE_FORMAT_C14X2:
            return width * height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB565:
            return width * height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            return width * height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            return width * height * 4;

        case IMGFMT::TPL_IMAGE_FORMAT_CMPR:
            return width * height / 2;
    }

    return 0;
}

