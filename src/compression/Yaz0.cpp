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

#define HEADER_MAGIC 0x307A6159 // 'Yaz0'

#define CHUNKS_PER_GROUP 8
#define MAX_MATCH_LENGTH 0xFF + 0x12
#define ZLIB_MIN_MATCH 3

struct Yaz0Header {
    // Magic string (should always equal to "Yaz0" if valid)
    char magic[4];

    // Size of the file before compression in bytes
    uint32_t uncompressedSize;

    // Reserved bytes. reserved[0] in newer Yaz0 files is used for data alignment but
    // Rhythm Heaven Fever doesn't use the newer revision.
    uint32_t reserved[2];
} __attribute__((packed));

namespace Yaz0 {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel) {
    std::vector<unsigned char> result(sizeof(Yaz0Header));
    result.reserve(dataSize);

    Yaz0Header* header = reinterpret_cast<Yaz0Header*>(result.data());

    *reinterpret_cast<uint32_t*>(header->magic) = HEADER_MAGIC;

    header->uncompressedSize = BYTESWAP_32(static_cast<uint32_t>(dataSize));

    header->reserved[0] = 0x0000;
    header->reserved[1] = 0x0000;

    // Compression
    {
        struct CompressionState {
            std::vector<unsigned char>& buffer;

            unsigned pendingChunks{ 0 };

            unsigned groupHeaderOffset{ sizeof(Yaz0Header) };
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
                        unsigned distance = dist - 1;
                        unsigned length = lc + ZLIB_MIN_MATCH;

                        if (length < 18) {
                            compressionState->buffer.push_back(static_cast<unsigned char>(
                                ((length - 2) << 4) | static_cast<unsigned char>(distance >> 8)
                            ));
                            compressionState->buffer.push_back(static_cast<unsigned char>(distance));
                        }
                        else {
                            // If the match is longer than 18 bytes, 3 bytes are needed to write the match
                            const unsigned actualLength = std::min<unsigned>(MAX_MATCH_LENGTH, length);

                            compressionState->buffer.push_back(static_cast<unsigned char>(distance >> 8));
                            compressionState->buffer.push_back(static_cast<unsigned char>(distance));
                            compressionState->buffer.push_back(static_cast<unsigned char>(actualLength - 0x12));
                        }
                    }

                    if (++compressionState->pendingChunks == CHUNKS_PER_GROUP) {
                        // Write group header
                        compressionState->buffer[compressionState->groupHeaderOffset] =
                            static_cast<char>(compressionState->groupHeader.to_ulong());

                        // Reset values
                        compressionState->pendingChunks = 0;
                        compressionState->groupHeader.reset();
                        compressionState->groupHeaderOffset = static_cast<uint32_t>(compressionState->buffer.size());

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

std::optional<std::vector<unsigned char>> decompress(const unsigned char* compressedData, const size_t dataSize) {
    if (dataSize < sizeof(Yaz0Header)) {
        std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: data size smaller than header size!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(compressedData);

    uint32_t uncompressedSize = BYTESWAP_32(header->uncompressedSize);

    if (*reinterpret_cast<const uint32_t*>(header->magic) != HEADER_MAGIC) {
        std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: header magic failed check!\n";
        return std::nullopt; // return nothing (std::optional)
    }

    std::vector<unsigned char> destination(uncompressedSize);

    // compressedData without header
    const unsigned char* dataSource = compressedData + sizeof(Yaz0Header);

    uint32_t readOffset{ 0 };
    uint32_t destOffset{ 0 };

    uint8_t bitsLeft{ 0 };
    uint8_t opcodeByte{ 0 };

    while (destOffset < uncompressedSize) {
        // Read new opcode byte if no bits left
        if (bitsLeft == 0) {
            opcodeByte = *(dataSource + (readOffset++));

            // Reset bitsLeft (1byte)
            bitsLeft = 8;
        }

        if (UNLIKELY(readOffset >= dataSize)) {
            // We've reached the end of the uncompressed file but we're not finished reading yet;
            // Usually this means the file is invalid.

            std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: the read offset is larger than the data size!\n";
            std::cerr << "[Yaz0::uncompress] The Yaz0 binary might be corrupted.\n";
            return std::nullopt; // return nothing (std::optional)
        }

        if ((opcodeByte & 0b10000000) != 0) { // MSB set: Copy one byte directly.
            destination.at(destOffset++) = *(dataSource + (readOffset++));

            if (readOffset >= (size_t)(compressedData - sizeof(Yaz0Header))) {
                return std::nullopt;
            }
        }
        else { // MSB not set: RLE compression.
            uint8_t byteA = *(dataSource + (readOffset++));
            uint8_t byteB = *(dataSource + (readOffset++));

            uint32_t dist = ((byteA & 0xF) << 8) | byteB;

            int copySource = destOffset - (dist + 1);

            if (UNLIKELY(copySource < 0)) {
                std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: the destination offset is out of bounds (negative)!\n";
                std::cerr << "[Yaz0::uncompress] The Yaz0 binary might be corrupted.\n";
                return std::nullopt; // return nothing (std::optional)
            }

            unsigned numBytes = byteA >> 4;
            if (numBytes == 0)
                numBytes = *(dataSource + (readOffset++)) + 0x12;
            else
                numBytes += 2;

            if (numBytes > uncompressedSize - destOffset)
                numBytes = uncompressedSize - destOffset;

            for (unsigned i = 0; i < numBytes; i++, copySource++, destOffset++)
                destination[destOffset] = destination[copySource];
        }

        opcodeByte <<= 1;
        bitsLeft--;
    }

    return destination;
}

} // namespace Yaz0
