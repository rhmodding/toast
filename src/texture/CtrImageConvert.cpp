#include "CtrImageConvert.hpp"

#include <vector>

#include <thread>

#include "../Logging.hpp"

#include "../rg_etc1/rg_etc1.h"

#include "../ConfigManager.hpp"

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

static void IMPLEMENTATION_FROM_ETC1A4(unsigned char* _result, unsigned srcWidth, unsigned srcHeight, const unsigned char* data) {
    uint32_t* result = reinterpret_cast<uint32_t*>(_result);

    unsigned readOffset { 0 };
	
    for (unsigned yy = 0; yy < srcHeight; yy += 8) {
        for (unsigned xx = 0; xx < srcWidth; xx += 8) {
            for (unsigned z = 0; z < 4; z++) {
                unsigned xStart = (z == 1 || z == 3) ? 4 : 0;
                unsigned yStart = (z == 2 || z == 3) ? 4 : 0;

                uint32_t pixels[4 * 4];

                uint64_t alphaData = *(uint64_t*)(data + readOffset);
                readOffset += 8;

                // Nintendo stores their ETC1 blocks in little- instead of big-endian.
                uint64_t blockData = BYTESWAP_64(*(uint64_t*)(data + readOffset));
                readOffset += 8;

                rg_etc1::unpack_etc1_block(&blockData, pixels, true);

                // Alpha pass.
                for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
                    for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
                        // 4bit -> 8bit
                        result[(y * srcWidth) + x] = ((alphaData & 0xF) << 24) | ((alphaData & 0xF) << 28);

                        alphaData >>= 4;
                    }
                }

                // Color pass (ETC1 block).
                uint32_t* currentPixel = pixels;
                for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
                    for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
                        result[(y * srcWidth) + x] |= (*(currentPixel++) & 0x00FFFFFF);
                    }
                }
            }
        }
    }
}

/*
    TO implementations (RGBA32 to x)
*/

static void IMPLEMENTATION_TO_ETC1A4(unsigned char* result, unsigned srcWidth, unsigned srcHeight, const unsigned char* _data) {
    rg_etc1::pack_etc1_block_init();
    
    rg_etc1::etc1_pack_params packerParams;
    packerParams.m_quality = static_cast<rg_etc1::etc1_quality>(
        std::min(ConfigManager::getInstance().getConfig().etc1Quality, ETC1Quality_High)
    );
    packerParams.m_dithering = false;

    const uint32_t* data = reinterpret_cast<const uint32_t*>(_data);

    // ETC1 packing is extremely slow, so to speed things up we opt to parallelize
    // the packing process.

    std::vector<std::thread> threads;

    unsigned numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 4;
    
    unsigned chunkHeight = (srcHeight + numThreads - 1) / numThreads;

    auto worker = [result, data, srcWidth, srcHeight, &packerParams](unsigned yStart, unsigned yEnd) {
        unsigned writeOffset = srcWidth * yStart;

        for (unsigned yy = yStart; yy < yEnd; yy += 8) {
            for (unsigned xx = 0; xx < srcWidth; xx += 8) {
                for (unsigned z = 0; z < 4; z++) {
                    unsigned xStart = (z == 1 || z == 3) ? 4 : 0;
                    unsigned yStart = (z == 2 || z == 3) ? 4 : 0;

                    uint32_t pixels[4 * 4];

                    uint64_t* alphaData = (uint64_t*)(result + writeOffset);
                    writeOffset += 8;

                    uint64_t* blockData = (uint64_t*)(result + writeOffset);
                    writeOffset += 8;

                    // Fill pixels for packing.
                    uint32_t* currentPixel = pixels;
                    for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
                        for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
                            *currentPixel = data[(y * srcWidth) + x];
                            *currentPixel |= 0xFF000000;
                            currentPixel++;
                        }
                    }

                    // Write alpha block.
                    *alphaData = 0;
                    for (unsigned x = xx + xStart; x < xx + xStart + 4; x++) {
                        for (unsigned y = yy + yStart; y < yy + yStart + 4; y++) {
                            uint64_t alpha4 = (data[(y * srcWidth) + x] >> 24) / 17;
                            *alphaData = (*alphaData >> 4) | (alpha4 << 60);
                        }
                    }

                    rg_etc1::pack_etc1_block(blockData, pixels, packerParams);

                    // Nintendo stores their ETC1 blocks in little- instead of big-endian.
                    *blockData = BYTESWAP_64(*blockData);
                }
            }
        }
    };

    for (unsigned i = 0; i < numThreads; i++) {
        unsigned yStart = i * chunkHeight;
        unsigned yEnd = std::min(yStart + chunkHeight, srcHeight);
        if (yStart >= srcHeight)
            break;

        threads.emplace_back(worker, yStart, yEnd);
    }

    for (auto& thread : threads)
        thread.join();
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
        Logging::err << "[CtrImageConvert::toRGBA32] Cannot convert texture: invalid format (" << format << ')' << std::endl;
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
	unsigned tilesX = (width + 3) / 4;
   	unsigned tilesY = (height + 3) / 4;

    // ETC1 block + alpha block
   	return tilesX * tilesY * (8 + 8);
}

static unsigned ImageByteSize_ETC1(unsigned width, unsigned height) {
	unsigned tilesX = (width + 3) / 4;
   	unsigned tilesY = (height + 3) / 4;

   	return tilesX * tilesY * 8;
}

unsigned CtrImageConvert::getImageByteSize(const CTPK::CTPKImageFormat type, unsigned width, unsigned height, unsigned mipCount) {
    unsigned sum = 0;

    switch (type) {
    case ImageFormat::CTPK_IMAGE_FORMAT_ETC1A4: {
        for (unsigned i = 0; i < mipCount; i++) {
            sum += ImageByteSize_ETC1A4(width, height);

            width = (width > 1) ? width / 2 : 1;
            height = (height > 1) ? height / 2 : 1;
        }
    } break;
    case ImageFormat::CTPK_IMAGE_FORMAT_ETC1: {
        for (unsigned i = 0; i < mipCount; i++) {
            sum += ImageByteSize_ETC1(width, height);

            width = (width > 1) ? width / 2 : 1;
            height = (height > 1) ? height / 2 : 1;
        }
    } break;

    default:
        Logging::err << "[CtrImageConvert::getImageByteSize] Invalid format passed (" << static_cast<int>(type) << ')' << std::endl;
        break;
    }

    return sum;
}

unsigned CtrImageConvert::getImageByteSize(const CTPK::CTPKTexture& texture) {
    return getImageByteSize(texture.format, texture.width, texture.height, texture.mipCount);
}
