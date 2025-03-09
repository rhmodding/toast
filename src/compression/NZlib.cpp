#include "NZlib.hpp"

#include "../Logging.hpp"

#include <algorithm>

#include <zlib-ng.h>

#include "../common.hpp"

struct NZLibHeader {
    uint32_t inflateSize; // In BE.
    unsigned char deflatedData[0];
} __attribute__((packed));

namespace NZlib {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    std::vector<unsigned char> deflated(
        sizeof(NZLibHeader) + zng_compressBound(dataSize)
    );

    *reinterpret_cast<NZLibHeader*>(deflated.data()) = NZLibHeader {
        .inflateSize = BYTESWAP_32(dataSize)
    };
    
    zng_stream strm {};
    strm.next_in = data;
    strm.avail_in = dataSize;
    strm.next_out = deflated.data() + sizeof(NZLibHeader);
    strm.avail_out = deflated.size() - sizeof(NZLibHeader);

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

    deflated.resize(sizeof(NZLibHeader) + strm.total_out);

    float reductionRate = ((dataSize - strm.total_out) / static_cast<float>(dataSize)) * 100.f;

    Logging::info <<
        "[NZlib::compress] Successfully compressed " << (dataSize / 1000) << "kb of data down to " <<
        (strm.total_out / 1000) << "kb (" << reductionRate << "% reduction)." << std::endl;

    return deflated;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(NZLibHeader)) {
        Logging::err << "[NZlib::decompress] Invalid NZlib binary: data size smaller than header size!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    const NZLibHeader* header = reinterpret_cast<const NZLibHeader*>(data);

    const uint32_t deflateSize = dataSize - sizeof(NZLibHeader);
    const uint32_t inflateSize = BYTESWAP_32(header->inflateSize);

    if (inflateSize == 0) {
        Logging::err << "[NZlib::decompress] Invalid inflate size!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    std::vector<unsigned char> inflated(inflateSize);

    zng_stream strm {};
    strm.next_in = header->deflatedData;
    strm.avail_in = deflateSize;
    strm.next_out = inflated.data();
    strm.avail_out = inflateSize;

    const int init = zng_inflateInit2(&strm, 15);
    if (init != Z_OK) {
        Logging::err << "[NZlib::decompress] zng_inflateInit failed!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    const int ret = zng_inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        Logging::err << "[NZlib::decompress] zng_inflate failed: " << ret << std::endl;

        zng_inflateEnd(&strm);
        return std::nullopt; // return nothing (std::optional)
    }

    zng_inflateEnd(&strm);
    return inflated;
}

bool checkDataValid(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(NZLibHeader))
        return false;
    
    const NZLibHeader* header = reinterpret_cast<const NZLibHeader*>(data);

    // Endianness doesn't matter here
    if (header->inflateSize == 0)
        return false;
 
    // Compression method & flags. Should always be equal to 0x78
    // for gzip-compressed data.
    const uint8_t cmf = header->deflatedData[0];

    // The CMF & FLG bytes in MSB order. Should always be a multiple
    // of 31.
    const uint16_t checkv = BYTESWAP_16(
        *reinterpret_cast<const uint16_t*>(header->deflatedData)
    );

    if (cmf != 0x78 || (checkv % 31) != 0)
        return false;
    
    return true;
}

} // namespace NZlib
