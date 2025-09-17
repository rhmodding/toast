#include "NZlib.hpp"

#include <algorithm>

#include <chrono>

#include <zlib-ng.h>

#include "Logging.hpp"

#include "Macro.hpp"

constexpr unsigned int MIN_ZLIB_DATA_SIZE = 6;

namespace NZlib {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    if (dataSize > 0xFFFFFFFF) {
        Logging::error("[NZlib::compress] Unable to compress: size of data is more than 4GiB!");
        return std::nullopt; // return nothing (std::optional)
    }
    if (dataSize == 0) {
        Logging::error("[NZlib::compress] Unable to compress: size of data is zero");
        return std::nullopt; // return nothing (std::optional)
    }

    auto compressStartTime = std::chrono::high_resolution_clock::now();

    std::vector<unsigned char> deflated(
        sizeof(uint32_t) + zng_compressBound(dataSize)
    );

    *reinterpret_cast<uint32_t*>(deflated.data()) = BYTESWAP_32(static_cast<uint32_t>(dataSize));

    unsigned char* dest = deflated.data() + sizeof(uint32_t);
    size_t destLen = deflated.size() - sizeof(uint32_t);

    int compResult = zng_compress2(dest, &destLen, data, dataSize, compressionLevel);
    if (compResult != Z_OK) {
        Logging::error("[NZlib::compress] zng_compress2 failed (code {})!", compResult);
        return std::nullopt; // return nothing (std::optional)
    }

    deflated.resize(sizeof(uint32_t) + destLen);

    auto compressTotalTime = std::chrono::high_resolution_clock::now() - compressStartTime;

    auto compressTotalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(compressTotalTime).count();

    float reductionRate = ((dataSize - destLen) / static_cast<float>(dataSize)) * 100.f;

    Logging::info(
        "[NZlib::compress] Successfully compressed {}kb of data down to {}kb ({}% reduction) in {}ms.",
        dataSize / 1024,
        destLen / 1024,
        reductionRate,
        compressTotalTimeMs
    );

    return deflated;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(uint32_t)) {
        Logging::error("[NZlib::decompress] Invalid NZlib binary: data size smaller than header size!");
        return std::nullopt;
    }
    if (dataSize > (0xFFFFFFFF + sizeof(uint32_t))) {
        Logging::error("[NZlib::decompress] Invalid NZlib binary: size of compressed payload is more than 4GiB!");
        return std::nullopt;
    }

    const uint32_t deflateSize = dataSize - sizeof(uint32_t);
    if (deflateSize < MIN_ZLIB_DATA_SIZE || deflateSize > dataSize) {
        Logging::error("[NZlib::decompress] The length of the compressed data is invalid!");
        return std::nullopt;
    }

    const uint32_t inflateSize = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(data));
    if (inflateSize == 0) {
        Logging::error("[NZlib::decompress] Invalid inflate size!");
        return std::nullopt;
    }

    auto decompressStartTime = std::chrono::high_resolution_clock::now();

    std::vector<unsigned char> inflated(inflateSize);

    unsigned char* dest = inflated.data();
    size_t destLen = inflated.size();

    const unsigned char* src = data + sizeof(uint32_t);
    size_t srcLen = deflateSize;

    int decompResult = zng_uncompress2(dest, &destLen, src, &srcLen);
    if (decompResult != Z_OK) {
        Logging::error("[NZlib::decompress] zng_uncompress2 failed (code {})!", decompResult);
        return std::nullopt; // return nothing (std::optional)
    }

    auto decompressEndTime = std::chrono::high_resolution_clock::now() - decompressStartTime;

    auto decompressWorkTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(decompressEndTime).count();

    if (srcLen < deflateSize) {
        size_t padLength = deflateSize - srcLen;
        Logging::warn("[NZlib::decompress] Compressed data is padded by {} bytes; strange..", padLength);
    }

    Logging::info(
        "[NZlib::decompress] Decompressed {}kb of data in {}ms from {}kb of compressed data.",
        inflateSize / 1024,
        decompressWorkTimeMs,
        dataSize / 1024
    );

    return inflated;
}

bool checkDataValid(const unsigned char* data, const size_t dataSize) {
    if (dataSize < (sizeof(uint32_t) + MIN_ZLIB_DATA_SIZE))
        return false;

    const uint32_t inflateSize = *reinterpret_cast<const uint32_t*>(data);

    // Endianness doesn't matter here
    if (inflateSize == 0)
        return false;

    const uint8_t* zlibData = data + sizeof(uint32_t);
 
    // Compression method & flags. Should always be equal to 0x78
    // for gzip-compressed data.
    const uint8_t cmf = zlibData[0];

    // The CMF & FLG bytes in MSB order. Should always be a multiple
    // of 31.
    const uint16_t checkv = BYTESWAP_16(*reinterpret_cast<const uint16_t*>(zlibData));

    if (cmf != 0x78 || (checkv % 31) != 0)
        return false;

    return true;
}

} // namespace NZlib
