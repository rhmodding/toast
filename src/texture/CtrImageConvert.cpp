#include "CtrImageConvert.hpp"

#include "../rg_etc1/rg_etc1.h"

#include "../common.hpp"

typedef CTPK::CTPKImageFormat ImageFormat;

typedef void (*FromImplementation)(unsigned char*, unsigned, unsigned, const unsigned char*);
typedef void (*ToImplementation)(unsigned char*, unsigned, unsigned, const unsigned char*);

/*
    FROM implementations (x to RGBA32)
*/

static void IMPLEMENTATION_FROM_RGBA4444(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
	unsigned readOffset { 0 };

    for (unsigned yy = 0; yy < srcHeight; yy += 8) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {

            for (unsigned y = 0; y < 8; y++) {
                if (yy + y >= srcHeight) break;

                const unsigned rowBase = srcWidth * (yy + y);

                for (unsigned x = 0; x < 8; x++) {
                    if (xx + x >= srcWidth) break;

                    const unsigned writeOffset = (rowBase + xx + x) * 4;

                    const uint16_t sourcePixel = *reinterpret_cast<const uint16_t*>(
                        data + readOffset + (y * 2 * 8) + (x * 2)
                    );

                    result[writeOffset + 0] = ((sourcePixel >> 12) & 0xf) * 16;
                    result[writeOffset + 1] = ((sourcePixel >>  8) & 0xf) * 16;
                    result[writeOffset + 2] = ((sourcePixel >>  4) & 0xf) * 16;
                    result[writeOffset + 3] = ((sourcePixel >>  0) & 0xf) * 16;
                }
            }
            readOffset += 2 * 8 * 8;
        }
    }
}

static void IMPLEMENTATION_FROM_ETC1A4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    unsigned readOffset { 0 };

	uint32_t* resultWhole = reinterpret_cast<uint32_t*>(result);
	
    // O_O
	for (unsigned xx = 0; xx < srcHeight; xx += 8) {
		for (unsigned yy = 0; yy < srcWidth; yy += 8) {
			for (unsigned z = 0; z < 4; z++) {
				unsigned xStart = (z & 2) ? 4 : 0;
                unsigned yStart = (z & 1) ? 4 : 0;

                uint64_t alphaData = *(uint64_t*)(data + readOffset);
				readOffset += 8;

				for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
					for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
						unsigned pixelIdx = (x * srcWidth) + y;

						resultWhole[pixelIdx] = ((alphaData & 0xF) << 24) | ((alphaData & 0xF) << 28);
						alphaData >>= 4;
					}
                }

				uint64_t blockData = BYTESWAP_64(*(uint64_t*)(data + readOffset));
				readOffset += 8;

                uint32_t pixels[4 * 4];

				rg_etc1::unpack_etc1_block(&blockData, pixels);

				const uint32_t* currentPixel = pixels;
				for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
					for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
						unsigned pixelIdx = (x * srcWidth) + y;
                        
						resultWhole[pixelIdx] |= (*(currentPixel++) & 0xFFFFFF);
					}
				}
			}
		}
	}
}

/*
    TO implementations (RGBA32 to x)
*/

static void IMPLEMENTATION_TO_ETC1A4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    rg_etc1::pack_etc1_block_init();

    rg_etc1::etc1_pack_params packerParams;
    packerParams.m_quality = rg_etc1::cHighQuality;
    packerParams.m_dithering = false;
    
    unsigned writeOffset { 0 };

	uint32_t* resultWhole = reinterpret_cast<uint32_t*>(result);
	
    // O_O
	for (unsigned xx = 0; xx < srcHeight; xx += 8) {
		for (unsigned yy = 0; yy < srcWidth; yy += 8) {
			for (unsigned z = 0; z < 4; z++) {
				unsigned xStart = (z & 2) ? 4 : 0;
                unsigned yStart = (z & 1) ? 4 : 0;

                uint32_t pixels[4 * 4];

                uint64_t* alphaData = (uint64_t*)(data + writeOffset);
				writeOffset += 8;

                unsigned currentAlphaShift = 0;
                unsigned currentPixel = 0;

                // Fill alpha data and fill pixels at the same time.
                for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
                    for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
                        unsigned pixelIdx = (x * srcWidth) + y;
                        unsigned alphaValue = result[(pixelIdx * 4) + 3] / 17;

                        *alphaData |= (alphaValue << currentAlphaShift);
                        currentAlphaShift += 4;

                        pixels[currentPixel] = resultWhole[pixelIdx];
                        currentPixel++;
                    }
                }

                uint64_t* blockData = (uint64_t*)(data + writeOffset);
				writeOffset += 8;

				rg_etc1::pack_etc1_block(blockData, pixels, packerParams);
                *blockData = BYTESWAP_64(*blockData);
			}
		}
	}
}

bool CtrImageConvert::toRGBA32(
    unsigned char* buffer,
    const CTPK::CTPKImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data
) {
    FromImplementation implementation;
    switch (format) {
	case ImageFormat::CTPK_IMAGE_FORMAT_RGBA4444:
		implementation = IMPLEMENTATION_FROM_RGBA4444;
		break;
	
    case ImageFormat::CTPK_IMAGE_FORMAT_ETC1A4:
        implementation = IMPLEMENTATION_FROM_ETC1A4;
        break;

    default:
        std::cerr << "[CtrImageConvert::toRGBA32] Cannot convert texture: invalid format (" << format << ")\n";
        return false;
    }

    implementation(buffer, srcWidth, srcHeight, data);

    return true;
}

bool CtrImageConvert::toRGBA32(
    CTPK::CTPKTexture& texture,
    const unsigned char* data
) {
    return CtrImageConvert::toRGBA32(
        texture.data.data(),
        texture.format,
        texture.width, texture.height,
        data
    );
}

bool CtrImageConvert::fromRGBA32(
    unsigned char* buffer,
    const CTPK::CTPKImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data
) {
    ToImplementation implementation;
    switch (format) {
    case ImageFormat::CTPK_IMAGE_FORMAT_ETC1A4:
        implementation = IMPLEMENTATION_TO_ETC1A4;
        break;

    default:
        return false;
    }

    implementation(buffer, srcWidth, srcHeight, data);

    return true;
}

bool CtrImageConvert::fromRGBA32(
    const CTPK::CTPKTexture& texture,
    unsigned char* buffer
) {
    return CtrImageConvert::fromRGBA32(
        buffer,
        texture.format,
        texture.width, texture.height,
        texture.data.data()
    );
}

static unsigned ImageByteSize_ETC1A4(unsigned width, unsigned height) {
	unsigned tilesX = (width + 7) / 8;
   	unsigned tilesY = (height + 7) / 8;

   	return tilesX * tilesY * 64;
}

unsigned CtrImageConvert::getImageByteSize(const CTPK::CTPKImageFormat type, unsigned width, unsigned height, unsigned mipCount) {
    unsigned sum;
    
    switch (type) {
    case ImageFormat::CTPK_IMAGE_FORMAT_ETC1A4: {
        for (unsigned i = 0; i < mipCount; i++) {
            sum += ImageByteSize_ETC1A4(width, height);
        
            width = (width > 1) ? width / 2 : 1;
            height = (height > 1) ? height / 2 : 1;
        }
    }

    default:
        std::cerr << "[CtrImageConvert::getImageByteSize] Invalid format passed (" << (int)type << ")\n";
        sum = 0;
    }

    return sum;
}

unsigned CtrImageConvert::getImageByteSize(const CTPK::CTPKTexture& texture) {
    return getImageByteSize(texture.format, texture.width, texture.height, texture.mipCount);
}
