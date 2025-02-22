#include "NZlib.hpp"

#include <iostream>

#include <zlib-ng.h>

#include "../common.hpp"

struct NZLibHeader {
    uint32_t inflateSize; // In BE.
    unsigned char deflatedData[0];
} __attribute__((packed));

namespace NZlib {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    std::vector<unsigned char> deflated(zng_compressBound(dataSize));
    
    zng_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = dataSize;
    strm.next_out = deflated.data();
    strm.avail_out = deflated.size();

    if (zng_deflateInit(&strm, compressionLevel) != Z_OK) {
        std::cerr << "[NZlib::compress] zng_deflateInit failed!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    int ret = zng_deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        std::cerr << "[NZlib::decompress] zng_deflate failed: " << ret << '\n';

        zng_deflateEnd(&strm);
        return std::nullopt; // return nothing (std::optional)
    }

    zng_deflateEnd(&strm);

    deflated.resize(strm.total_out);

    return deflated;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(NZLibHeader)) {
        std::cerr << "[NZlib::decompress] Invalid NZlib binary: data size smaller than header size!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    const NZLibHeader* header = reinterpret_cast<const NZLibHeader*>(data);

    const uint32_t deflateSize = dataSize - sizeof(NZLibHeader);
    const uint32_t inflateSize = BYTESWAP_32(header->inflateSize);

    std::vector<unsigned char> inflated(inflateSize);

    zng_stream strm = {0};
    strm.next_in = header->deflatedData;
    strm.avail_in = deflateSize;
    strm.next_out = inflated.data();
    strm.avail_out = inflateSize;

    if (zng_inflateInit(&strm) != Z_OK) {
        std::cerr << "[NZlib::decompress] zng_inflateInit failed!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    int ret = zng_inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        std::cerr << "[NZlib::decompress] zng_inflate failed: " << ret << '\n';

        zng_inflateEnd(&strm);
        return std::nullopt; // return nothing (std::optional)
    }

    zng_inflateEnd(&strm);
    return inflated;
}

} // namespace NZlib
