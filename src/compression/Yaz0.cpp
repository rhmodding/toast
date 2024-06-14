/*
    * This file contains functions derived from syaz0 by zeldamods,
    * which is licensed under the GPL 2.0 license.
    * 
    * Original source: https://github.com/zeldamods/syaz0
    * 
    * For more details about the GPL 2.0 license, see the LICENSE file.
*/

#include "Yaz0.hpp"

#include "../common.hpp"

#include <cstdint>

#include <iostream>
#include <fstream>

#include <optional>

#include <string>
#include <vector>
#include <bitset>

#include <algorithm>

#include <zlib-ng.h>

#define CHUNKS_PER_GROUP 8
#define MAX_MATCH_LENGTH 0xFF + 0x12
#define ZLIB_MIN_MATCH 3

#pragma pack(push, 1) // Pack struct members tightly without padding

struct Yaz0Header {
    // Magic string (should always equal to "Yaz0" if valid)
    char magic[4];

    // Size of the file before compression in bytes
    uint32_t uncompressedSize;

    // Reserved bytes. reserved[0] in newer Yaz0 files is used for data alignment but
    // Rhythm Heaven Fever doesn't use the newer revision.
    uint32_t reserved[2];
};

#pragma pack(pop) // Reset packing alignment to default

namespace Yaz0 {
    std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, uint8_t compressionLevel) {
        std::vector<unsigned char> result(sizeof(Yaz0Header));
        result.reserve(dataSize);

        Yaz0Header* header = reinterpret_cast<Yaz0Header*>(result.data());

        header->magic[0] = 'Y';
        header->magic[1] = 'a';
        header->magic[2] = 'z';
        header->magic[3] = '0';
        header->uncompressedSize = BYTESWAP_32(static_cast<uint32_t>(dataSize));
        header->reserved[0] = 0x0000;
        header->reserved[1] = 0x0000;

        // Compression
        {
            struct CompressionData {
                uint32_t pendingChunks{ 0 };
                std::bitset<8> groupHeader;
                uint32_t groupHeaderOffset{ sizeof(Yaz0Header) }; // Skip header

                std::vector<unsigned char>& buffer;

                CompressionData(std::vector<unsigned char>& buffer) : buffer(buffer) {}
            } compressionData(result);

            result.push_back(0xFFu);

            {
                unsigned char dummy[8];
                size_t dummySize = 8;
                
                const int zlibRun = zng_compress2(
                    dummy, &dummySize, data, dataSize, std::clamp<int>(compressionLevel, 6, 9),
                    [](void* data, uint32_t dist, uint32_t lc) {
                        auto* compressionData = reinterpret_cast<CompressionData*>(data);

                        if (dist == 0) {
                            compressionData->groupHeader.set(7 - compressionData->pendingChunks);
                            compressionData->buffer.push_back(static_cast<unsigned char>(lc));
                        }
                        else {
                            uint32_t distance = dist - 1;
                            uint32_t length = lc + ZLIB_MIN_MATCH;

                            if (length < 18) {
                                compressionData->buffer.push_back(static_cast<unsigned char>(
                                    ((length - 2) << 4) | static_cast<unsigned char>(distance >> 8)
                                ));
                                compressionData->buffer.push_back(static_cast<unsigned char>(distance));
                            }
                            else {
                                // If the match is longer than 18 bytes, 3 bytes are needed to write the match
                                const uint32_t actualLength = std::min<uint32_t>(MAX_MATCH_LENGTH, length);

                                compressionData->buffer.push_back(static_cast<unsigned char>(distance >> 8));
                                compressionData->buffer.push_back(static_cast<unsigned char>(distance));
                                compressionData->buffer.push_back(static_cast<unsigned char>(actualLength - 0x12));
                            }
                        }

                        if (++compressionData->pendingChunks == CHUNKS_PER_GROUP) {
                            // Write group header
                            compressionData->buffer[compressionData->groupHeaderOffset] =
                                static_cast<char>(compressionData->groupHeader.to_ulong());
                            
                            // Reset values
                            compressionData->pendingChunks = 0;
                            compressionData->groupHeader.reset();
                            compressionData->groupHeaderOffset = static_cast<uint32_t>(compressionData->buffer.size());

                            compressionData->buffer.push_back(0xFFu);
                        }
                    },
                    &compressionData
                );

                if (zlibRun != Z_OK) {
                    std::cerr << "[Yaz0::compress] zng_compress failed with code " << zlibRun << ".\n";
                    return std::nullopt; // return nothing (std::optional)
                }

                if (compressionData.pendingChunks != 0)
                    compressionData.buffer[compressionData.groupHeaderOffset] =
                        static_cast<unsigned char>(compressionData.groupHeader.to_ulong());
            }
        }

        return result;
    }

    std::optional<std::vector<unsigned char>> decompress(const unsigned char* compressedData, const size_t dataSize) {
        const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(compressedData);

        uint32_t uncompressedSize = BYTESWAP_32(header->uncompressedSize);

        if (std::strncmp(header->magic, "Yaz0", 4) != 0) {
            std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: header magic failed strncmp!\n";
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

            if (readOffset >= dataSize) {
                // We've reached the end of the uncompressed file but we're not finished reading yet;
                // Usually this means the file is invalid.

                std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: the read offset is larger than the data size!\n";
                std::cout << "[Yaz0::uncompress] The Yaz0 binary might be corrupted.\n";
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

                if (readOffset >= (size_t)(compressedData - sizeof(Yaz0Header)))
                    return std::nullopt;

                uint32_t dist = ((byteA & 0xF) << 8) | byteB;
                if ((destOffset - (dist + 1)) < 0) {
                    std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: the destination offset is out of bounds (negative)!\n";
                    std::cout << "[Yaz0::uncompress] The Yaz0 binary might be corrupted.\n";
                    return std::nullopt; // return nothing (std::optional)
                }

                uint32_t copySource = destOffset - (dist + 1);

                uint32_t numBytes = byteA >> 4;

                if (numBytes == 0) {
                    numBytes = *(dataSource + (readOffset++));
                    numBytes += 0x12;
                }
                else
                    numBytes += 2;

                for (uint32_t i = 0; i < numBytes; i++, copySource++, destOffset++) {
                    if (destOffset < uncompressedSize)
                        destination[destOffset] = destination[copySource];
                }
            }

            opcodeByte <<= 1;
            bitsLeft--;
        }

        return destination;
    }
} // namespace Yaz0