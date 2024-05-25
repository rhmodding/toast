#include "ImageConvert.hpp"

#include "TPL.hpp"

#include "../common.hpp"

#define IMGFMT TPL::TPLImageFormat

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

void IMPLEMENTATION_FROM_I4(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 8); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 8); xBlock++) {

            for (uint8_t pY = 0; pY < 8; pY++) {
                for (uint8_t pX = 0; pX < 8; pX += 2) {
                    if ((xBlock * 8 + pX >= srcWidth) || (yBlock * 8 + pY >= srcHeight))
                        continue;

                    const uint8_t intensityA = ((data[readOffset] & 0xF0) >> 4) * 0x11;
                    const uint8_t intensityB = (data[readOffset] & 0x0F) * 0x11;

                    readOffset++;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 8) + pY) + (xBlock * 8) + pX);

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
        }
    }
}

void IMPLEMENTATION_FROM_I8(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 8); xBlock++) {

            for (uint8_t pY = 0; pY < 4; pY++) {
                for (uint8_t pX = 0; pX < 8; pX++) {
                    if ((xBlock * 8 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    const uint8_t intensity = data[readOffset++];

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 8) + pX);

                    result[destIndex + 0] = intensity;
                    result[destIndex + 1] = intensity;
                    result[destIndex + 2] = intensity;
                    result[destIndex + 3] = 0xFFu;
                }
            }
        }
    }
}

void IMPLEMENTATION_FROM_IA4(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 8); xBlock++) {

            for (uint8_t pY = 0; pY < 4; pY++) {
                for (uint8_t pX = 0; pX < 8; pX++) {
                    if ((xBlock * 8 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 8) + pX);

                    const uint8_t alpha = ((data[readOffset] & 0xF0) >> 4) * 0x11;
                    const uint8_t intensity = (data[readOffset] & 0x0F) * 0x11;

                    readOffset++;

                    result[destIndex + 0] = intensity;
                    result[destIndex + 1] = intensity;
                    result[destIndex + 2] = intensity;
                    result[destIndex + 3] = alpha;
                }
            }
        }
    }
}

void IMPLEMENTATION_FROM_IA8(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };
    
    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 4); xBlock++) {

            for (uint8_t pY = 0; pY < 4; pY++) {
                for (uint8_t pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);

                    const uint8_t alpha = data[readOffset++];
                    const uint8_t intensity = data[readOffset++];

                    result[destIndex + 0] = intensity;
                    result[destIndex + 1] = intensity;
                    result[destIndex + 2] = intensity;
                    result[destIndex + 3] = alpha;
                }
            }
        }
    }
}

#pragma endregion

#pragma region RGBxxx

void IMPLEMENTATION_FROM_RGB565(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t yy = 0; yy < srcHeight; yy += 4) {
        for (uint16_t xx = 0; xx < srcWidth; xx += 4) {

            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth || yy + y >= srcHeight)
                        continue;

                    const uint32_t writeOffset = ((srcWidth * (yy + y)) + xx + x) * 4;

                    const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data.data() + readOffset));
                    readOffset += sizeof(uint16_t);

                    result[writeOffset + 0] = ((sourcePixel >> 11) & 0x1f) << 3;
                    result[writeOffset + 1] = ((sourcePixel >>  5) & 0x3f) << 2;
                    result[writeOffset + 2] = ((sourcePixel >>  0) & 0x1f) << 3;
                    result[writeOffset + 3] = 0xFFu;
                }
            }
        }
    }
}

void IMPLEMENTATION_FROM_RGB5A3(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t yy = 0; yy < srcHeight; yy += 4) {
        for (uint16_t xx = 0; xx < srcWidth; xx += 4) {  

            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if (xx + x >= srcWidth || yy + y >= srcHeight)
                        continue;

                    const uint32_t writeOffset = ((srcWidth * (yy + y)) + xx + x) * 4;

                    const uint16_t sourcePixel = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(data.data() + readOffset));
                    readOffset += sizeof(uint16_t);

                    if ((sourcePixel & 0x8000) != 0) { // RGB555
                        result[writeOffset + 0] = (((sourcePixel >> 10) & 0x1f) << 3) | (((sourcePixel >> 10) & 0x1f) >> 2);
                        result[writeOffset + 1] = (((sourcePixel >> 5) & 0x1f) << 3) | (((sourcePixel >> 5) & 0x1f) >> 2);
                        result[writeOffset + 2] = (((sourcePixel) & 0x1f) << 3) | (((sourcePixel) & 0x1f) >> 2);

                        result[writeOffset + 3] = 0xFFu;
                    }
                    else { // RGBA4443
                        result[writeOffset + 3] =
                            (((sourcePixel >> 12) & 0x7) << 5) | (((sourcePixel >> 12) & 0x7) << 2) |
                            (((sourcePixel >> 12) & 0x7) >> 1);

                        result[writeOffset + 0] = (((sourcePixel >> 8) & 0xf) << 4) | ((sourcePixel >> 8) & 0xf);
                        result[writeOffset + 1] = (((sourcePixel >> 4) & 0xf) << 4) | ((sourcePixel >> 4) & 0xf);
                        result[writeOffset + 2] = (((sourcePixel) & 0xf) << 4) | ((sourcePixel) & 0xf);
                    }
                }
            }
        }
    }
}

void IMPLEMENTATION_FROM_RGBA32(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 4); xBlock++) {

            for (uint8_t pY = 0; pY < 4; pY++) {
                for (uint8_t pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);
                    result[destIndex + 3] = data[readOffset++]; // alpha
                    result[destIndex + 0] = data[readOffset++]; // red
                }
            }

            for (uint8_t pY = 0; pY < 4; pY++) {
                for (uint8_t pX = 0; pX < 4; pX++) {
                    if ((xBlock * 4 + pX >= srcWidth) || (yBlock * 4 + pY >= srcHeight))
                        continue;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + pY) + (xBlock * 4) + pX);
                    result[destIndex + 1] = data[readOffset++]; // green
                    result[destIndex + 2] = data[readOffset++]; // blue
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
    result.resize(srcWidth * srcHeight * 4);

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

uint32_t ImageConvert::getImageByteSize(const TPL::TPLImageFormat type, uint16_t width, uint16_t height) {
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

