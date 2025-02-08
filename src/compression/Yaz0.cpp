/*
    * This file contains code derived from syaz0 by zeldamods,
    * which is licensed under the GPL 2.0 license.
    *
    * Original source: https://github.com/zeldamods/syaz0
    *
    * For more details about the GPL 2.0 license, see the LICENSE file.
*/

#include "Yaz0.hpp"

#include <bitset>

#include <algorithm>

#include <iostream>

#include <zlib-ng.h>

#include "../common.hpp"

constexpr uint32_t YAZ0_MAGIC = IDENTIFIER_TO_U32('Y', 'a', 'z', '0');

constexpr unsigned int CHUNKS_PER_GROUP = 8;
constexpr unsigned int MAX_MATCH_LENGTH = 0xFFu + 0x12u;
constexpr unsigned int ZLIB_MIN_MATCH = 3;

struct Yaz0Header {
    // Magic value (should always equal to "Yaz0" if valid).
    // Compare to HEADER_MAGIC
    uint32_t magic { YAZ0_MAGIC };

    // Size of the file after decompression (in bytes).
    uint32_t decompressedSize;

    // Reserved bytes. reserved[0] in newer Yaz0 files is used for data alignment but
    // Rhythm Heaven Fever doesn't use the newer revision.
    uint32_t reserved[2] { 0x00000000, 0x00000000 };
} __attribute__((packed));

namespace Yaz0 {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    std::vector<unsigned char> result(sizeof(Yaz0Header));
    result.reserve(dataSize);

    Yaz0Header* header = reinterpret_cast<Yaz0Header*>(result.data());
    *header = Yaz0Header {
        .decompressedSize = BYTESWAP_32(static_cast<uint32_t>(dataSize)),
    };

    // Compression using zlib-ng to find matches.
    if (compressionLevel != 0) {
        struct CompressionState {
            std::vector<unsigned char>& buffer;

            unsigned pendingChunks { 0 };

            unsigned operationsOffset { sizeof(Yaz0Header) };
            std::bitset<8> operations;

            CompressionState(std::vector<unsigned char>& buffer) : buffer(buffer) {}
        } compressionState(result);

        result.push_back(0xFFu);

        {
            unsigned char dummy[8];
            size_t dummySize = 8;

            const int zlibRun = zng_compress2(
                dummy, &dummySize, data, dataSize, std::clamp<int>(compressionLevel, 2, 9),
                [](void* usrData, unsigned dist, unsigned lc) {
                    CompressionState* compressionState = reinterpret_cast<CompressionState*>(usrData);

                    if (dist == 0) {
                        compressionState->operations.set(7 - compressionState->pendingChunks);
                        compressionState->buffer.push_back(static_cast<unsigned char>(lc));
                    }
                    else {
                        const unsigned newDistance = dist - 1;
                        const unsigned matchLength = lc + ZLIB_MIN_MATCH;

                        if (matchLength < 18) {
                            const unsigned char firstByte = static_cast<unsigned char>(
                                ((matchLength - 2) << 4) | (newDistance >> 8)
                            );

                            compressionState->buffer.push_back(firstByte);
                            compressionState->buffer.push_back(static_cast<unsigned char>(newDistance));
                        }
                        else {
                            // If the match is longer than 18 bytes, 3 bytes are needed to write the match
                            const unsigned actualLength = std::min<unsigned>(MAX_MATCH_LENGTH, matchLength);

                            compressionState->buffer.push_back(static_cast<unsigned char>(newDistance >> 8));
                            compressionState->buffer.push_back(static_cast<unsigned char>(newDistance));
                            compressionState->buffer.push_back(static_cast<unsigned char>(actualLength - 0x12));
                        }
                    }

                    if (++compressionState->pendingChunks == CHUNKS_PER_GROUP) {
                        compressionState->buffer[compressionState->operationsOffset] =
                            static_cast<unsigned char>(compressionState->operations.to_ulong());

                        compressionState->pendingChunks = 0;
                        compressionState->operations.reset();
                        compressionState->operationsOffset = static_cast<unsigned>(compressionState->buffer.size());

                        compressionState->buffer.push_back(0xFFu);
                    }
                }, &compressionState
            );

            if (zlibRun != Z_OK) {
                std::cerr << "[Yaz0::compress] zng_compress failed with code " << zlibRun << ".\n";
                return std::nullopt; // return nothing (std::optional)
            }

            if (compressionState.pendingChunks != 0)
                compressionState.buffer[compressionState.operationsOffset] =
                    static_cast<unsigned char>(compressionState.operations.to_ulong());
        }
    }
    // Direct store. Result will be 12.5% bigger than the original data since one
    // op byte is needed for every 8 bytes.
    else {
        // One op byte for every 8 bytes.
        unsigned compressedSize = dataSize + ((dataSize + 7) / 8);
        unsigned fullSize = sizeof(Yaz0Header) + compressedSize;

        result.resize(fullSize);

        const unsigned char* srcEnd = data + dataSize;
        const unsigned char* dstEnd = result.data() + fullSize;

        const unsigned char* currentSrc = data;
        unsigned char* currentDst = result.data() + sizeof(Yaz0Header);

        while (currentDst < dstEnd && currentSrc < srcEnd) {
            *(currentDst++) = 0xFF; // All bits to 1.

            // Copy 8 bytes.
            *(uint64_t*)currentDst = *(uint64_t*)currentSrc;
            currentSrc += 8; currentDst += 8;
        }
    }

    return result;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(Yaz0Header)) {
        std::cerr << "[Yaz0::decompress] Invalid Yaz0 binary: data size smaller than header size!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(data);
    if (header->magic != YAZ0_MAGIC) {
        std::cerr << "[Yaz0::decompress] Invalid Yaz0 binary: header magic is nonmatching!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    const uint32_t decompressedSize = BYTESWAP_32(header->decompressedSize);

    std::vector<unsigned char> destination(decompressedSize);

    const unsigned char* srcByte = data + sizeof(Yaz0Header);

    const unsigned char* dstStart = destination.data();
    const unsigned char* dstEnd = destination.data() + decompressedSize;

    uint8_t opByte, opMask = 0;

    for (
        unsigned char* dstByte = destination.data();
        dstByte < dstEnd; opMask >>= 1
    ) {
        // No more operation bits left. Refresh
        if (opMask == 0) {
            opByte = *(srcByte++);
            opMask = (1 << 7);
        }

        if (opByte & opMask) // Copy one byte.
            *(dstByte++) = *(srcByte++);
        else { // Run-length data.
            int distToDest = (*srcByte << 8) | *(srcByte + 1);
            srcByte += 2;

            int runSrcIdx = (dstByte - dstStart) - (distToDest & 0xfff);

            int runLen = ((distToDest >> 12) == 0) ?
                *(srcByte++) + 0x12 :
                (distToDest >> 12) + 2;

            for (; runLen > 0; runLen--, dstByte++, runSrcIdx++) {
                if (UNLIKELY(dstByte >= dstEnd)) {
                    std::cout << "[Yaz0::decompress] The Yaz0 data is malformed. The binary might be corrupted.\n";
                    return std::nullopt;
                }

                *dstByte = destination.data()[runSrcIdx - 1];
            }
        }
    }

    return destination;
}

} // namespace Yaz0
