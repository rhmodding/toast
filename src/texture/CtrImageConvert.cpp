#include "CtrImageConvert.hpp"

#include "../rg_etc1/rg_etc1.h"

#include "../common.hpp"

typedef CTPK::CTPKImageFormat ImageFormat;

typedef void (*FromImplementation)(unsigned char*, unsigned, unsigned, const unsigned char*);
typedef void (*ToImplementation)(unsigned char*, unsigned*, unsigned, unsigned, const unsigned char*);

inline int deswizzlePixelIdx(unsigned index, unsigned width, unsigned height) {
    unsigned row = index / width;
    unsigned col = index % width;

    unsigned newRow = width - 1 - col;
    unsigned newCol = row;

    return newRow * height + newCol;
}

static void IMPLEMENTATION_FROM_RGBA4444(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
	// TODO
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

static unsigned ImageByteSize_ETC1A4(unsigned width, unsigned height) {
	unsigned tilesX = (width + 7) / 8;
   	unsigned tilesY = (height + 7) / 8;

   	return tilesX * tilesY * 64;
}

unsigned CtrImageConvert::getImageByteSize(const CTPK::CTPKImageFormat type, const unsigned width, const unsigned height) {
    switch (type) {
    case ImageFormat::CTPK_IMAGE_FORMAT_ETC1A4:
        return ImageByteSize_ETC1A4(width, height);

    default:
        std::cerr << "[CtrImageConvert::getImageByteSize] Invalid format passed (" << (int)type << ")\n";
        return 0;
    }

    // NOT REACHED
}

unsigned CtrImageConvert::getImageByteSize(const CTPK::CTPKTexture& texture) {
    return getImageByteSize(texture.format, texture.width, texture.height);
}
