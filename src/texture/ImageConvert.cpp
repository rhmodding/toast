#include "ImageConvert.hpp"

#include "TPL.hpp"

#include "../common.hpp"

#include <unordered_map>

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
            // The block data is split down into two subblocks:
            //    Subblock A: Alpha and Red channel
            //    Subblock B: Green and Blue channel

            // Subblock A
            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if ((xBlock * 4 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + y) + (xBlock * 4) + x);

                    result[destIndex + 3] = data[readOffset++]; // Alpha channel
                    result[destIndex + 0] = data[readOffset++]; // Red channel
                }
            }

            // Subblock B
            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if ((xBlock * 4 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + y) + (xBlock * 4) + x);

                    result[destIndex + 1] = data[readOffset++]; // Green channel
                    result[destIndex + 2] = data[readOffset++]; // Blue channel
                }
            }
        }
    }
}

#pragma endregion

#pragma region Cx

void IMPLEMENTATION_FROM_C8(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data, uint32_t* colorPalette) {
    uint32_t readOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 8); xBlock++) {

            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    if ((xBlock * 8 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t color = colorPalette[*(reinterpret_cast<const uint8_t*>(data.data()) + readOffset++)];
                    const uint32_t destIndex = 4 * (srcWidth * ((yBlock * 4) + y) + (xBlock * 8) + x);

                    result[destIndex + 0] = (color >> 24) & 0xFFu;
                    result[destIndex + 1] = (color >> 16) & 0xFFu;
                    result[destIndex + 2] = (color >>  8) & 0xFFu;
                    result[destIndex + 3] = color & 0xFFu;
                }
            }
        }
    }
}

#pragma endregion

#pragma region CMPR

void IMPLEMENTATION_FROM_CMPR(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t readOffset{ 0 };

    for (uint16_t y = 0; y < srcHeight; y += 4) {
        for (uint16_t x = 0; x < srcWidth; x += 4) {
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

            for (uint8_t iy = 0; iy < 4; ++iy) {
                for (uint8_t ix = 0; ix < 4; ++ix) {
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

void IMPLEMENTATION_TO_RGB5A3(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t writeOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 4); xBlock++) {  

            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if ((xBlock * 4 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint8_t* readPixel =
                        reinterpret_cast<const uint8_t*>(data.data()) +
                        (((srcWidth * ((yBlock * 4) + y)) + ((xBlock * 4) + x)) * 4);

                    // A pixel is 16-bit but we write it in two 8-bit segments.
                    uint8_t* pixel = reinterpret_cast<uint8_t*>(&result[writeOffset]);
                    writeOffset += sizeof(uint16_t);

                    if (readPixel[3] == 255) { // Opaque, write RGB555 pixel
                        // Bits: 
                        // 1    RRRRRGGGGG BBBBB
                        // ^    ^    ^     ^
                        // Type Red  Green Blue

                        *(pixel + 0) = 0x0080 | ((readPixel[0] & 0xF8) >> 1) | (readPixel[1] >> 6);
                        *(pixel + 1) = ((readPixel[1] & 0x38) << 2) | (readPixel[2] >> 3);
                    } else { // Transparent, write RGBA4443
                        // Bits: 
                        // 0    AAA   RRRRGGGG  BBBB
                        // ^    ^     ^   ^     ^
                        // Type Alpha Red Green Blue

                        *(pixel + 0) = ((readPixel[3] >> 1) & 0x70) | ((readPixel[0] & 0xF0) >> 4);                                                        
                        *(pixel + 1) = (readPixel[1] & 0xF0) | ((readPixel[2] & 0xF0) >> 4);	
                    }
                }
            }
        }
    }
}

void IMPLEMENTATION_TO_RGBA32(std::vector<char>& result, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
    uint32_t writeOffset = 0;

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 4); xBlock++) {
            // The block data is split down into two subblocks:
            //    Subblock A: Alpha and Red channel
            //    Subblock B: Green and Blue channel

            // Subblock A
            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if ((xBlock * 4 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t readOffset = 4 * (srcWidth * ((yBlock * 4) + y) + (xBlock * 4) + x);

                    result[writeOffset++] = data[readOffset + 3]; // Alpha channel
                    result[writeOffset++] = data[readOffset + 0]; // Red channel
                }
            }

            // Subblock B
            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if ((xBlock * 4 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t readOffset = 4 * (srcWidth * ((yBlock * 4) + y) + (xBlock * 4) + x);

                    result[writeOffset++] = data[readOffset + 1]; // Green channel
                    result[writeOffset++] = data[readOffset + 2]; // Blue channel
                }
            }

        }
    }
}

#pragma endregion

#pragma region Cx

void IMPLEMENTATION_TO_C8(std::vector<char>& result, std::vector<uint32_t>& colors, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
                       // Color  // Index
    std::unordered_map<uint32_t, uint8_t> indexMap;
    for (uint32_t i = 0; i < colors.size(); i++)
        indexMap[colors.at(i)] = static_cast<uint8_t>(i);

    uint32_t writeOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 8); xBlock++) {

            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    if ((xBlock * 8 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t readPixel = *(
                        reinterpret_cast<const uint32_t*>(data.data()) +
                        (srcWidth * ((yBlock * 4) + y) + (xBlock * 8) + x)
                    );

                    uint8_t* pixel = reinterpret_cast<uint8_t*>(&result[writeOffset++]);

                    // TODO: fix colors not being found
                    if (indexMap.find(readPixel) != indexMap.end()) {
                        *pixel = indexMap[readPixel];
                    }
                }
            }
        }
    }
}

void IMPLEMENTATION_TO_C14X2(std::vector<char>& result, std::vector<uint32_t>& colors, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data) {
                       // Color  // Index (byteswapped)
    std::unordered_map<uint32_t, uint16_t> indexMap;
    for (uint32_t i = 0; i < colors.size(); i++)
        indexMap[colors.at(i)] = BYTESWAP_16(static_cast<uint16_t>(i));

    uint32_t writeOffset{ 0 };

    for (uint16_t yBlock = 0; yBlock < (srcHeight / 4); yBlock++) {
        for (uint16_t xBlock = 0; xBlock < (srcWidth / 4); xBlock++) {  

            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    if ((xBlock * 4 + x >= srcWidth) || (yBlock * 4 + y >= srcHeight))
                        continue;

                    const uint32_t readPixel = *(
                        reinterpret_cast<const uint32_t*>(data.data()) +
                        ((srcWidth * ((yBlock * 4) + y)) + ((xBlock * 4) + x))
                    );

                    uint16_t* pixel = reinterpret_cast<uint16_t*>(&result[writeOffset]);
                    writeOffset += sizeof(uint16_t);

                    if (indexMap.find(readPixel) != indexMap.end()) {
                        *pixel = indexMap[readPixel];
                    }
                }
            }
        }
    }
} 

#pragma endregion

#pragma endregion

//////////////////////////////////////////////////////////////////

bool ImageConvert::toRGBA32(
    std::vector<char>& buffer,
    const TPL::TPLImageFormat type,
    const uint16_t srcWidth,
    const uint16_t srcHeight,
    const std::vector<char>& data,
    uint32_t* colorPalette
) {
    buffer.resize(srcWidth * srcHeight * 4);

    switch (type) {
        case IMGFMT::TPL_IMAGE_FORMAT_I4: 
            IMPLEMENTATION_FROM_I4(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_I8:
            IMPLEMENTATION_FROM_I8(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_IA4:
            IMPLEMENTATION_FROM_IA4(buffer, srcWidth, srcHeight, data);
            break;
        
        case IMGFMT::TPL_IMAGE_FORMAT_IA8:
            IMPLEMENTATION_FROM_IA8(buffer, srcWidth, srcHeight, data);
            break;
        
        case IMGFMT::TPL_IMAGE_FORMAT_RGB565:
            IMPLEMENTATION_FROM_RGB565(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            IMPLEMENTATION_FROM_RGB5A3(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            IMPLEMENTATION_FROM_RGBA32(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            if (!colorPalette) {
                std::cerr << "[ImageConvert::toRGBA32] Couldn't convert C8 format: no color palette passed.\n";
                return false;
            }
            
            IMPLEMENTATION_FROM_C8(buffer, srcWidth, srcHeight, data, colorPalette);
            break;

        // TODO: Palette-based formats

        case IMGFMT::TPL_IMAGE_FORMAT_CMPR:
            IMPLEMENTATION_FROM_CMPR(buffer, srcWidth, srcHeight, data);
            break;

        default:
            return false;
    }

    return true;
}

bool ImageConvert::fromRGBA32(
    std::vector<char>& buffer,
    const TPL::TPLImageFormat type,
    const uint16_t srcWidth,
    const uint16_t srcHeight,
    const std::vector<char>& data,
    std::vector<uint32_t>* colorPalette
) {
    switch (type) {
        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            IMPLEMENTATION_TO_RGB5A3(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            IMPLEMENTATION_TO_RGBA32(buffer, srcWidth, srcHeight, data);
            break;

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            if (!colorPalette) {
                std::cerr << "[ImageConvert::fromRGBA32] Couldn't convert to C8 format: no color palette passed.\n";
                return false;
            }

            IMPLEMENTATION_TO_C8(buffer, *colorPalette, srcWidth, srcHeight, data);
            break;
        case IMGFMT::TPL_IMAGE_FORMAT_C14X2:
            if (!colorPalette) {
                std::cerr << "[ImageConvert::fromRGBA32] Couldn't convert to C14X2 format: no color palette passed.\n";
                return false;
            }

            IMPLEMENTATION_TO_C14X2(buffer, *colorPalette, srcWidth, srcHeight, data);
            break;
        
        default:
            return false;
    }
    
    return true;
}

uint32_t ImageConvert::getImageByteSize(const TPL::TPLImageFormat type, const uint16_t width, const uint16_t height) {
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

uint32_t ImageConvert::getImageByteSize(const TPL::TPLTexture& texture) {
    switch (texture.format) {
        case IMGFMT::TPL_IMAGE_FORMAT_I4:
            return texture.width * texture.height / 2;

        case IMGFMT::TPL_IMAGE_FORMAT_I8:
            return texture.width * texture.height;

        case IMGFMT::TPL_IMAGE_FORMAT_IA4:
            return texture.width * texture.height;

        case IMGFMT::TPL_IMAGE_FORMAT_IA8:
            return texture.width * texture.height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_C4:
            return texture.width * texture.height / 2;

        case IMGFMT::TPL_IMAGE_FORMAT_C8:
            return texture.width * texture.height;

        case IMGFMT::TPL_IMAGE_FORMAT_C14X2:
            return texture.width * texture.height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB565:
            return texture.width * texture.height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_RGB5A3:
            return texture.width * texture.height * 2;

        case IMGFMT::TPL_IMAGE_FORMAT_RGBA32:
            return texture.width * texture.height * 4;

        case IMGFMT::TPL_IMAGE_FORMAT_CMPR:
            return texture.width * texture.height / 2;
    }

    return 0;
}