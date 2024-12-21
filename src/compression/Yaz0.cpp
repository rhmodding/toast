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

#define HEADER_MAGIC IDENTIFIER_TO_U32('Y', 'a', 'z', '0')

#define CHUNKS_PER_GROUP (8)
#define MAX_MATCH_LENGTH (0xFF + 0x12)
#define ZLIB_MIN_MATCH (3)

struct Yaz0Header {
    // Magic value (should always equal to "Yaz0" if valid)
    // Compare to HEADER_MAGIC
    uint32_t magic { HEADER_MAGIC };

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

    // Compression
    {
        struct CompressionState {
            std::vector<unsigned char>& buffer;

            unsigned pendingChunks { 0 };

            unsigned groupHeaderOffset { sizeof(Yaz0Header) };
            std::bitset<8> groupHeader;

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
                        compressionState->groupHeader.set(7 - compressionState->pendingChunks);
                        compressionState->buffer.push_back(static_cast<unsigned char>(lc));
                    }
                    else {
                        unsigned newDistance = dist - 1;
                        unsigned matchLength = lc + ZLIB_MIN_MATCH;

                        if (matchLength < 18) {
                            unsigned char firstByte = static_cast<unsigned char>(
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
                        // Write group header
                        compressionState->buffer[compressionState->groupHeaderOffset] =
                            static_cast<unsigned char>(compressionState->groupHeader.to_ulong());

                        // Reset values
                        compressionState->pendingChunks = 0;
                        compressionState->groupHeader.reset();
                        compressionState->groupHeaderOffset = static_cast<unsigned>(compressionState->buffer.size());

                        compressionState->buffer.push_back(0xFFu);
                    }
                }, &compressionState
            );

            if (zlibRun != Z_OK) {
                std::cerr << "[Yaz0::compress] zng_compress failed with code " << zlibRun << ".\n";
                return std::nullopt; // return nothing (std::optional)
            }

            if (compressionState.pendingChunks != 0)
                compressionState.buffer[compressionState.groupHeaderOffset] =
                    static_cast<unsigned char>(compressionState.groupHeader.to_ulong());
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
    if (header->magic != HEADER_MAGIC) {
        std::cerr << "[Yaz0::decompress] Invalid Yaz0 binary: header magic failed check!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    const uint32_t decompressedSize = BYTESWAP_32(header->decompressedSize);

    std::vector<unsigned char> destination(decompressedSize);

    const unsigned char* const dataEnd = data + dataSize;

    const unsigned char* currentByte = data + sizeof(Yaz0Header);

    unsigned destOffset { 0 };

    unsigned bitsLeft { 0 };
    uint8_t headerByte { 0 };

    while (destOffset < decompressedSize) {
        // All bits read from prev header byte; get next header byte.
        if (bitsLeft == 0) {
            headerByte = *(currentByte++);
            bitsLeft = 8;
        }

        if (UNLIKELY(currentByte >= dataEnd)) {
            std::cerr << "[Yaz0::decompress] Invalid Yaz0 binary: the read offset is larger than the data size!\n";
            std::cerr << "[Yaz0::decompress] The Yaz0 binary might be corrupted.\n";
            return std::nullopt; // return nothing (std::optional)
        }

        if ((headerByte & (1 << 7)) != 0) { // Bit set: Copy one byte directly.
            destination[destOffset++] = *(currentByte++);
        }
        else { // Bit not set: Run-length encoding.
            uint8_t byteA = *(currentByte++);
            uint8_t byteB = *(currentByte++);

            unsigned dist = ((byteA & 0xF) << 8) | byteB;

            int copySource = destOffset - (dist + 1);

            if (UNLIKELY(copySource < 0)) {
                std::cerr << "[Yaz0::decompress] Invalid Yaz0 binary: the destination offset is out of bounds (negative)!\n";
                std::cerr << "[Yaz0::decompress] The Yaz0 binary might be corrupted.\n";
                return std::nullopt; // return nothing (std::optional)
            }

            unsigned numBytes = byteA >> 4;
            if (numBytes == 0)
                numBytes = *(currentByte++) + 0x12;
            else
                numBytes += 2;

            for (unsigned i = 0; i < numBytes && destOffset < decompressedSize; i++)
                destination[destOffset++] = destination[copySource++];
        }

        // Shift over to next bit.
        headerByte <<= 1;
        bitsLeft--;
    }

    return destination;
}

} // namespace Yaz0
