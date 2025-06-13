#include "APNGHack.hpp"

#include <cstdint>
#include <cstring>

#include "util/CRC32Util.hpp"

#include <zlib-ng.h>

#include "Macro.hpp"

enum class PNGColorType : uint8_t {
    Grayscale      = 0, // Luminance values (bpp is bitDepth).
    Truecolor      = 2, // RGB values (bpp is bitDepth*3).
    Indexed        = 3, // Palette indices (bpp is bitDepth).
    GrayscaleAlpha = 4, // Luminance + alpha values (bpp is bitDepth*2).
    TruecolorAlpha = 6, // RGBA values (bpp is bitDepth*4).
};

enum class PNGCompressionMethod : uint8_t {
    Zlib = 0 // Image data is compressed using Zlib deflate.
};

enum class PNGFilterMethod : uint8_t {
    None    = 0,
    Sub     = 1,
    Up      = 2,
    Average = 3,
    Paeth   = 4
};

enum class PNGInterlaceMethod : uint8_t {
    None  = 0,
    Adam7 = 1
};

constexpr uint32_t PNG_MAGIC_0 = IDENTIFIER_TO_U32(0x89,'P','N','G');
constexpr uint32_t PNG_MAGIC_1 = IDENTIFIER_TO_U32(0x0D,0x0A,0x1A,0x0A);

constexpr uint32_t PNG_IHDR_ID = IDENTIFIER_TO_U32('I','H','D','R');
constexpr uint32_t PNG_IDAT_ID = IDENTIFIER_TO_U32('I','D','A','T');
constexpr uint32_t PNG_IEND_ID = IDENTIFIER_TO_U32('I','E','N','D');

constexpr uint32_t PNG_ACTL_ID = IDENTIFIER_TO_U32('a','c','T','L');
constexpr uint32_t PNG_FCTL_ID = IDENTIFIER_TO_U32('f','c','T','L');

// The first frame must have the identifier "IDAT". All consecutive frames
// must have the identifier "fdAT".
constexpr uint32_t PNG_FDAT_ID = IDENTIFIER_TO_U32('f','d','A','T');

struct PNGDummyChunk {
    uint32_t chunkDataSize; // Exclusive of this field, the identifier and the CRC.
    uint8_t crcStart[0];
    uint32_t chunkIdentifier;
    uint8_t chunkData[0];
} __attribute__((packed));

struct PNGFileHeader {
    uint32_t magic0 { PNG_MAGIC_0 }, magic1 { PNG_MAGIC_1 };
    PNGDummyChunk firstChunk[0];
} __attribute__((packed));

struct PNGIhdrChunk {
    uint32_t chunkDataSize { BYTESWAP_32(13) };
    uint8_t crcStart[0];
    uint32_t chunkIdentifier { PNG_IHDR_ID }; // Compare to PNG_IHDR_ID
    uint8_t chunkData[0];

    uint32_t width, height;
    uint8_t bitDepth;
    PNGColorType colorType;
    PNGCompressionMethod compressionMethod;
    PNGFilterMethod filterMethod;
    PNGInterlaceMethod interlaceMethod;

    uint32_t crc[0];
} __attribute__((packed));

// https://wiki.mozilla.org/APNG_Specification
// ^^ this sucks REALLY bad but it's the only real APNG specification

struct PNGActlChunk {
    uint32_t chunkDataSize { BYTESWAP_32(8) };
    uint8_t crcStart[0];
    uint32_t chunkIdentifier { PNG_ACTL_ID }; // Compare to PNG_ACTL_ID
    uint8_t chunkData[0];

    uint32_t frameCount;
    uint32_t playCount; // 0 is infinite loop.

    uint32_t crc[0];
} __attribute__((packed));

// Specifies how the draw buffer is cleared when drawing a frame.
enum class APNGDisposeOp : uint8_t {
    None     = 0, // Don't do anything; leave the buffer as is.
    Clear    = 1, // Clear the buffer before drawing.
    Previous = 2, // Copy the previous frame before drawing.
};

enum class APNGBlendOp : uint8_t {
    Source = 0,
    Over   = 1
};

struct PNGFctlChunk {
    uint32_t chunkDataSize { BYTESWAP_32(26) };
    uint8_t crcStart[0];
    uint32_t chunkIdentifier { PNG_FCTL_ID }; // Compare to PNG_FCTL_ID
    uint8_t chunkData[0];

    uint32_t sequenceNumber;
    uint32_t frameW, frameH;
    uint32_t offsetX, offsetY;
    uint16_t delayNum, delayDen; // Fraction numerator and denominator.
    APNGDisposeOp disposeOpr;
    APNGBlendOp blendOpr;

    uint32_t crc[0];
} __attribute__((packed));

static const PNGDummyChunk* FindPNGChunk(const void* pngDataStart, const void* pngDataEnd, uint32_t targetIdentifier) {
    const PNGFileHeader* fileHeader = reinterpret_cast<const PNGFileHeader*>(pngDataStart);
    if (fileHeader->magic0 != PNG_MAGIC_0 || fileHeader->magic1 != PNG_MAGIC_1)
        return nullptr;

    const PNGDummyChunk* currentChunk = fileHeader->firstChunk;
    while (reinterpret_cast<const void*>(currentChunk) < pngDataEnd) {
        if (currentChunk->chunkIdentifier == targetIdentifier)
            return currentChunk;

        if (currentChunk->chunkIdentifier == PNG_IEND_ID)
            return nullptr;

        currentChunk = reinterpret_cast<const PNGDummyChunk*>(
            currentChunk->chunkData +
            BYTESWAP_32(currentChunk->chunkDataSize) + 4 // CRC field
        );
    }

    return nullptr;
}

std::vector<unsigned char> APNGHack::build(const BuildData& buildData) {
    unsigned finalSize = sizeof(PNGFileHeader) +
        sizeof(PNGIhdrChunk) + 4 +
        sizeof(PNGActlChunk) + 4 +
        sizeof(PNGDummyChunk) + 4; // IEND

    std::vector<std::vector<unsigned char>> dataList (buildData.frames.size());

    // Precompute size of APNG binary and prepare compressed image data.
    for (unsigned i = 0; i < buildData.frames.size(); i++) {
        const auto& frame = buildData.frames[i];

        std::vector<unsigned char> buffer (zng_compressBound(frame.rgbaData.size()));

        zng_stream strm {};
        strm.next_in = frame.rgbaData.data();
        strm.avail_in = frame.rgbaData.size();
        strm.next_out = buffer.data();
        strm.avail_out = buffer.size();

        zng_deflateInit2(&strm, Z_BEST_COMPRESSION, 8, 15, 8, 0);

        zng_deflate(&strm, Z_FINISH);

        buffer.resize(strm.total_out);

        zng_deflateEnd(&strm);

        finalSize += sizeof(PNGFctlChunk) + 4;
        // IDAT/fdAT chunk.
        finalSize += sizeof(PNGDummyChunk) + buffer.size() + 4;

        // fdAT chunks have an extra 4 bytes.
        if (i != 0)
            finalSize += 4;
    }

    std::vector<unsigned char> finalData(finalSize);

    PNGFileHeader* fileHeader = reinterpret_cast<PNGFileHeader*>(finalData.data());
    *fileHeader = PNGFileHeader {};

    PNGIhdrChunk* chunkIhdr = reinterpret_cast<PNGIhdrChunk*>(fileHeader->firstChunk);
    *chunkIhdr = PNGIhdrChunk {
        .width = BYTESWAP_32(buildData.canvasW),
        .height = BYTESWAP_32(buildData.canvasH),
        .bitDepth = 8,
        .colorType = PNGColorType::TruecolorAlpha,
        .compressionMethod = PNGCompressionMethod::Zlib,
        .filterMethod = PNGFilterMethod::None,
        .interlaceMethod = PNGInterlaceMethod::None
    };

    *chunkIhdr->crc = BYTESWAP_32(CRC32Util::compute(std::string_view(
        reinterpret_cast<char*>(chunkIhdr->crcStart),
        reinterpret_cast<char*>(chunkIhdr->crc)
    )));

    PNGActlChunk* chunkActl = reinterpret_cast<PNGActlChunk*>(chunkIhdr->crc + 1);
    *chunkActl = PNGActlChunk {
        .frameCount = BYTESWAP_32(static_cast<uint32_t>(buildData.frames.size())),
        .playCount = 0
    };

    *chunkActl->crc = BYTESWAP_32(CRC32Util::compute(std::string_view(
        reinterpret_cast<char*>(chunkActl->crcStart),
        reinterpret_cast<char*>(chunkActl->crc)
    )));

    PNGDummyChunk* currentChunk = reinterpret_cast<PNGDummyChunk*>(chunkActl->crc + 1);

    // Write fcTL & IDAT/fdAT chunks.
    for (unsigned i = 0; i < buildData.frames.size(); i++) {
        const auto& frame = buildData.frames[i];
        const bool isFirstFrame = i == 0;

        PNGFctlChunk* chunkFctl = reinterpret_cast<PNGFctlChunk*>(currentChunk);
        *chunkFctl = PNGFctlChunk {
            .sequenceNumber = BYTESWAP_32(i),
            .frameW = BYTESWAP_32(frame.width),
            .frameH = BYTESWAP_32(frame.height),
            .offsetX = BYTESWAP_32(frame.offsetX),
            .offsetY = BYTESWAP_32(frame.offsetY),
            .delayNum = BYTESWAP_16(frame.holdFrames),
            .delayDen = BYTESWAP_16(buildData.frameRate),
            .disposeOpr = APNGDisposeOp::None,
            .blendOpr = APNGBlendOp::Source
        };

        *chunkFctl->crc = BYTESWAP_32(CRC32Util::compute(std::string_view(
            reinterpret_cast<char*>(chunkFctl->crcStart),
            reinterpret_cast<char*>(chunkFctl->crc)
        )));

        PNGDummyChunk* destIdat = reinterpret_cast<PNGDummyChunk*>(chunkFctl->crc + 1);

        uint32_t srcDataSize = dataList[i].size();
        uint32_t dstDataSize = srcDataSize + (isFirstFrame ? 0 : 4);

        destIdat->chunkDataSize = BYTESWAP_32(dstDataSize);
        destIdat->chunkIdentifier = isFirstFrame ? PNG_IDAT_ID : PNG_FDAT_ID;

        uint8_t* dstImageDataStart = destIdat->chunkData + (isFirstFrame ? 0 : 4);
        memcpy(dstImageDataStart, dataList[i].data(), srcDataSize);

        // Write sequence number.
        if (!isFirstFrame) {
            uint32_t* destSequenceNumber = reinterpret_cast<uint32_t*>(destIdat->chunkData);

            // Sequence numbers are 1-indexed ONLY HERE for some reason.
            *destSequenceNumber = BYTESWAP_32(i + 1);
        }

        uint32_t* destCRC = reinterpret_cast<uint32_t*>(
            destIdat->chunkData + dstDataSize
        );

        *destCRC = BYTESWAP_32(CRC32Util::compute(std::string_view(
            reinterpret_cast<char*>(destIdat->crcStart),
            reinterpret_cast<char*>(destCRC)
        )));

        currentChunk = reinterpret_cast<PNGDummyChunk*>(destCRC + 1);
    }

    currentChunk->chunkDataSize = 0;
    currentChunk->chunkIdentifier = PNG_IEND_ID;
    *reinterpret_cast<uint32_t*>(currentChunk->chunkData) = BYTESWAP_32(CRC32Util::compute("IEND"));

    return finalData;
}
