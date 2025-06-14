/*
    * This file contains code derived from syaz0 by zeldamods,
    * which is licensed under the GPL 2.0 license.
    *
    * Original source: https://github.com/zeldamods/syaz0
    *
    * For more details about the GPL 2.0 license, see the LICENSE file.
*/

#include "Yaz0.hpp"

#include <cstdint>

#include <bitset>

#include <algorithm>

#include <iostream>

#include <chrono>

#include <zlib-ng.h>

#include "Logging.hpp"

#include "Macro.hpp"

constexpr uint32_t YAZ0_MAGIC = IDENTIFIER_TO_U32('Y', 'a', 'z', '0');

constexpr unsigned int CHUNKS_PER_GROUP = 8;
constexpr unsigned int MAX_MATCH_LENGTH = 0xFFu + 0x12u;
constexpr unsigned int ZLIB_MIN_MATCH = 3;

struct Yaz0Header {
    // Compare to YAZ0_MAGIC
    uint32_t magic { YAZ0_MAGIC };

    // Size of the file after decompression (in bytes).
    uint32_t decompressedSize;

    // Reserved bytes. reserved[0] in newer Yaz0 files is used for data alignment but
    // Rhythm Heaven Fever doesn't use the newer revision.
    uint32_t reserved[2] { 0x00000000, 0x00000000 };
} __attribute__((packed));

struct Yaz0CompressionState {
    std::vector<unsigned char>& buffer;

    unsigned pendingChunks { 0 };

    unsigned operationsOffset { sizeof(Yaz0Header) };
    std::bitset<8> operations;

    Yaz0CompressionState(std::vector<unsigned char>& buffer) :
        buffer(buffer)
    {}
};

namespace Yaz0 {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    // Header + every byte + all op bytes needed (no RLE, only copy).
    size_t maximumSize = sizeof(Yaz0Header) + dataSize + ((dataSize + 7) / 8);

    std::vector<unsigned char> result(sizeof(Yaz0Header));
    result.reserve(maximumSize);

    Yaz0Header* header = reinterpret_cast<Yaz0Header*>(result.data());
    *header = Yaz0Header {
        .decompressedSize = BYTESWAP_32(static_cast<uint32_t>(dataSize)),
    };

    // Compression using a modified version of zlib-ng to find RLE matches.
    if (compressionLevel != 0) {
        Yaz0CompressionState state(result);

        result.push_back(0xFFu);

        {
            unsigned char dummy[8];
            size_t dummySize = 8;

            const int zlibRun = zng_compress2(
                dummy, &dummySize, data, dataSize, std::clamp<int>(compressionLevel, 2, 9),
                [](void* usrData, unsigned dist, unsigned lc) {
                    Yaz0CompressionState* state = reinterpret_cast<Yaz0CompressionState*>(usrData);

                    if (dist == 0) {
                        state->operations.set(7 - state->pendingChunks);
                        state->buffer.push_back(static_cast<unsigned char>(lc));
                    }
                    else {
                        const unsigned newDistance = dist - 1;
                        const unsigned matchLength = lc + ZLIB_MIN_MATCH;

                        if (matchLength < 18) {
                            const unsigned char firstByte = static_cast<unsigned char>(
                                ((matchLength - 2) << 4) | (newDistance >> 8)
                            );

                            state->buffer.push_back(firstByte);
                            state->buffer.push_back(static_cast<unsigned char>(newDistance));
                        }
                        else {
                            // If the match is longer than 18 bytes, 3 bytes are needed to write the match
                            const unsigned actualLength = std::min<unsigned>(MAX_MATCH_LENGTH, matchLength);

                            state->buffer.push_back(static_cast<unsigned char>(newDistance >> 8));
                            state->buffer.push_back(static_cast<unsigned char>(newDistance));
                            state->buffer.push_back(static_cast<unsigned char>(actualLength - 0x12));
                        }
                    }

                    if (++state->pendingChunks == CHUNKS_PER_GROUP) {
                        state->buffer[state->operationsOffset] = static_cast<unsigned char>(
                            state->operations.to_ulong()
                        );

                        state->pendingChunks = 0;
                        state->operations.reset();
                        state->operationsOffset = static_cast<unsigned>(state->buffer.size());

                        state->buffer.push_back(0xFFu);
                    }
                }, &state
            );

            if (zlibRun != Z_OK) {
                Logging::err << "[Yaz0::compress] zng_compress failed with code " << zlibRun << '.' << std::endl;
                return std::nullopt; // return nothing (std::optional)
            }

            if (state.pendingChunks != 0) {
                state.buffer[state.operationsOffset] = static_cast<unsigned char>(
                    state.operations.to_ulong()
                );
            }
        }
    }
    // (Almost) store; result will be 12.5% bigger than the original data since one
    // op byte is needed for every 8 bytes. This should be used only to bypass
    // the usage of zlib-ng (for debugging purposes or otherwise), in any other case
    // this is super ultra horrible.
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

            // Copy 8 bytes directly.
            if ((srcEnd - currentSrc) >= 8) {
                *reinterpret_cast<uint64_t*>(currentDst) = *reinterpret_cast<const uint64_t*>(currentSrc);
                currentSrc += 8; currentDst += 8;
            }
            // Copy less than 8 bytes.
            else {
                unsigned nBytes = srcEnd - currentSrc;
                memcpy(currentDst, currentSrc, nBytes);

                currentSrc += nBytes; currentDst += nBytes;
            }
        }
    }

    size_t compressedSize = result.size() - sizeof(Yaz0Header);

    if (compressedSize >= dataSize) {
        Logging::info <<
            "[Yaz0::compress] Successfully stored " << (dataSize / 1000) << "kb of data (final size: " <<
            (compressedSize / 1000) << "kb)." << std::endl;
    }
    else {
        float reductionRate = ((dataSize - compressedSize) / static_cast<float>(dataSize)) * 100.f;
        Logging::info <<
            "[Yaz0::compress] Successfully compressed " << (dataSize / 1000) << "kb of data down to " <<
            (compressedSize / 1000) << "kb (" << reductionRate << "% reduction)." << std::endl;
    }

    return result;
}

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(Yaz0Header)) {
        Logging::err << "[Yaz0::decompress] Invalid Yaz0 binary: data size smaller than header size!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(data);
    if (header->magic != YAZ0_MAGIC) {
        Logging::err << "[Yaz0::decompress] Invalid Yaz0 binary: header magic is nonmatching!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    const uint32_t decompressedSize = BYTESWAP_32(header->decompressedSize);
    if (decompressedSize == 0) {
        Logging::err << "[Yaz0::decompress] Invalid Yaz0 binary: decompressed size is zero!" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

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
                    Logging::err << "[Yaz0::decompress] The Yaz0 data is malformed. The binary might be corrupted." << std::endl;
                    return std::nullopt; // return nothing (std::optional)
                }

                *dstByte = destination.data()[runSrcIdx - 1];
            }
        }
    }

    return destination;
}

bool checkDataValid(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(Yaz0Header))
        return false;

    const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(data);
    if (header->magic != YAZ0_MAGIC)
        return false;

    // Endianness doesn't matter here
    if (header->decompressedSize == 0)
        return false;

    return true;
}

} // namespace Yaz0
