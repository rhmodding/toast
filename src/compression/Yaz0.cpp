#include "Yaz0.hpp"

#include <cstdint>

#include <chrono>

#include "Yaz0/Compress.hpp"

#include "Logging.hpp"

#include "Macro.hpp"

constexpr uint32_t YAZ0_MAGIC = IDENTIFIER_TO_U32('Y','a','z','0');

struct Yaz0Header {
    // Compare to YAZ0_MAGIC.
    uint32_t magic { YAZ0_MAGIC };

    // Size of the file after decompression (in bytes).
    uint32_t decompressedSize;

    // Reserved bytes. reserved[0] in newer Yaz0 files is used for data alignment but
    // Rhythm Heaven Fever doesn't use the newer revision.
    uint32_t reserved[2] { 0x00000000, 0x00000000 };
} __attribute__((packed));

constexpr size_t MAX_MATCH_LENGTH = 0x111;

namespace Yaz0 {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    if (dataSize > 0xFFFFFFFF) {
        Logging::error("[Yaz0::compress] Unable to compress: size of data is more than 4GiB!");
        return std::nullopt; // return nothing (std::optional)
    }
    if (dataSize == 0) {
        Logging::error("[Yaz0::compress] Unable to compress: size of data is zero");
        return std::nullopt; // return nothing (std::optional)
    }

    std::vector<unsigned char> result(detail::MaxCompressedSize(dataSize));

    Yaz0Header* header = reinterpret_cast<Yaz0Header*>(result.data());
    *header = Yaz0Header {
        .decompressedSize = BYTESWAP_32(static_cast<uint32_t>(dataSize)),
    };

    auto compressStartTime = std::chrono::high_resolution_clock::now();

    size_t finalSize = detail::CompressImpl(data, dataSize, result.data());
    result.resize(finalSize);

    auto compressTotalTime = std::chrono::high_resolution_clock::now() - compressStartTime;

    auto compressTotalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(compressTotalTime).count();

    if (result.size() >= dataSize) {
        Logging::info(
            "[Yaz0::compress] Successfully stored {}kb of data in {}ms (final size: {}kb).",
            dataSize / 1024,
            compressTotalTimeMs,
            result.size() / 1024
        );
    }
    else {
        float reductionRate = ((dataSize - result.size()) / static_cast<float>(dataSize)) * 100.f;
        Logging::info(
            "[Yaz0::compress] Successfully compressed {}kb of data down to {}kb in {}ms ({}% reduction).",
            dataSize / 1024,
            result.size() / 1024,
            compressTotalTimeMs,
            reductionRate
        );
    }

    return result;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(Yaz0Header)) {
        Logging::error("[Yaz0::decompress] Invalid Yaz0 binary: data size smaller than header size!");
        return std::nullopt; // return nothing (std::optional)
    }

    const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(data);
    if (header->magic != YAZ0_MAGIC) {
        Logging::error("[Yaz0::decompress] Invalid Yaz0 binary: header magic is nonmatching!");
        return std::nullopt; // return nothing (std::optional)
    }

    const uint32_t decompressedSize = BYTESWAP_32(header->decompressedSize);
    if (decompressedSize == 0) {
        Logging::error("[Yaz0::decompress] Invalid Yaz0 binary: decompressed size is zero!");
        return std::nullopt; // return nothing (std::optional)
    }

    auto decompressStartTime = std::chrono::high_resolution_clock::now();

    std::vector<unsigned char> destination(decompressedSize);

    const unsigned char* srcByte = data + sizeof(Yaz0Header);

    unsigned char* dstStart = destination.data();
    unsigned char* dstEnd = destination.data() + decompressedSize;

    uint8_t opByte, opMask = 0;

    for (unsigned char* dstByte = dstStart; dstByte < dstEnd; opMask >>= 1) {
        // No more operation bits left; refresh.
        if (opMask == 0) {
            opByte = *(srcByte++);
            opMask = (1 << 7);
        }

        // Copy one byte.
        if (opByte & opMask) {
            *(dstByte++) = *(srcByte++);
        }
        // Run-length data.
        else {
            int distToDest = (*srcByte << 8) | *(srcByte + 1);
            srcByte += 2;

            int runSrcIdx = (dstByte - dstStart) - (distToDest & 0xfff);

            int runLen = ((distToDest >> 12) == 0) ?
                (*(srcByte++) + 0x12) : ((distToDest >> 12) + 2);

            for (; runLen > 0; runLen--, dstByte++, runSrcIdx++) {
                if (UNLIKELY(dstByte >= dstEnd)) {
                    Logging::error("[Yaz0::decompress] Invalid Yaz0 binary: run length is out of bounds!");
                    return std::nullopt; // return nothing (std::optional)
                }

                *dstByte = dstStart[runSrcIdx - 1];
            }
        }
    }

    auto decompressEndTime = std::chrono::high_resolution_clock::now() - decompressStartTime;

    auto decompressWorkTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(decompressEndTime).count();

    Logging::info(
        "[Yaz0::decompress] Decompressed {}kb of data in {}ms from {}kb of compressed data.",
        decompressedSize / 1024,
        decompressWorkTimeMs,
        dataSize / 1024
    );

    return destination;
}

bool checkDataValid(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(Yaz0Header))
        return false;

    const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(data);
    if (header->magic != YAZ0_MAGIC)
        return false;

    // We get to skip the byteswapping here.
    if (header->decompressedSize == 0)
        return false;

    return true;
}

} // namespace Yaz0
