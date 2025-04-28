#include "NZlib.hpp"

#include "../Logging.hpp"

#include <algorithm>

#include <zlib-ng.h>

#include "../common.hpp"

constexpr unsigned MIN_ZLIB_DATA_SIZE = 6;

namespace NZlib {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    std::vector<unsigned char> deflated(
        sizeof(uint32_t) + zng_compressBound(dataSize)
    );

    *reinterpret_cast<uint32_t*>(deflated.data()) = BYTESWAP_32(static_cast<uint32_t>(dataSize));

    zng_stream strm {};
    strm.next_in = data;
    strm.avail_in = dataSize;
    strm.next_out = deflated.data() + sizeof(uint32_t);
    strm.avail_out = deflated.size() - sizeof(uint32_t);
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // Hack!!! Blame zeldamods, not me!!!
    const int init = zng_deflateInit2(
        &strm, std::clamp<int>(compressionLevel, Z_NO_COMPRESSION, Z_BEST_COMPRESSION),
        8, 15, 8, 0
    );
    if (init != Z_OK) {
        Logging::err << "[NZlib::compress] zng_deflateInit failed!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    const int ret = zng_deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        Logging::err << "[NZlib::compress] zng_deflate failed: " << ret << std::endl;

        zng_deflateEnd(&strm);
        return std::nullopt; // return nothing (std::optional)
    }

    zng_deflateEnd(&strm);

    deflated.resize(sizeof(uint32_t) + strm.total_out);

    float reductionRate = ((dataSize - strm.total_out) / static_cast<float>(dataSize)) * 100.f;

    Logging::info <<
        "[NZlib::compress] Successfully compressed " << (dataSize / 1000) << "kb of data down to " <<
        (strm.total_out / 1000) << "kb (" << reductionRate << "% reduction)." << std::endl;

    return deflated;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(uint32_t)) {
        Logging::err << "[NZlib::decompress] Invalid NZlib binary: data size smaller than header size!" << std::endl;
        return std::nullopt;
    }

    const uint32_t deflateSize = dataSize - sizeof(uint32_t);
    if (deflateSize < MIN_ZLIB_DATA_SIZE || deflateSize > dataSize) {
        Logging::err << "[NZlib::decompress] The length of the compressed data is invalid!" << std::endl;
        return std::nullopt;
    }

    const uint32_t inflateSize = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(data));
    if (inflateSize == 0) {
        Logging::err << "[NZlib::decompress] Invalid inflate size!" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> inflated(inflateSize);

    zng_stream strm {};
    strm.next_in = data + sizeof(uint32_t);
    strm.avail_in = deflateSize;
    strm.next_out = inflated.data();
    strm.avail_out = inflateSize;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    const int init = zng_inflateInit2(&strm, 15);
    if (init != Z_OK) {
        Logging::err << "[NZlib::decompress] zng_inflateInit failed!" << std::endl;
        return std::nullopt;
    }

    const int ret = zng_inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        Logging::err << "[NZlib::decompress] zng_inflate failed: " << ret << std::endl;
        zng_inflateEnd(&strm);
        return std::nullopt;
    }

    zng_inflateEnd(&strm);
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
