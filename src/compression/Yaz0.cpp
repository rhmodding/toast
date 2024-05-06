#include "Yaz0.hpp"

#include "../common.hpp"

#include <cstdint>

#include <iostream>
#include <fstream>

#include <optional>

#include <string>
#include <vector>
#include <bitset>
#include <array>

#include <algorithm>

#include <zlib-ng.h>

constexpr const size_t ChunksPerGroup = 8;
constexpr const size_t MaximumMatchLength = 0xFF + 0x12;
constexpr const size_t WindowSize = 0x1000;
constexpr const uint32_t ZlibMinMatch = 3;

// The Yaz0 compression algorithm originates from
// https://github.com/zeldamods/syaz0

#pragma pack(push, 1) // Pack struct members tightly without padding

struct Yaz0Header {
    // Magic string (should always equal to "Yaz0" if valid)
    char magic[4];

    // Size of the file before compression in bytes
    uint32_t uncompressedSize;

    // Reserved bytes. reserved[0] in newer Yaz0 files are used for data alignment but
    // Rhythm Heaven Fever doesn't use the newer revision.
    uint32_t reserved[2];
};

#pragma pack(pop) // Reset packing alignment to default

namespace Yaz0 {
    std::optional<std::vector<char>> compress(const char* data, const size_t dataSize, uint8_t compressionLevel) {
        std::vector<char> result(sizeof(Yaz0Header));
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
                size_t pendingChunks{ 0 };
                std::bitset<8> groupHeader;
                std::size_t groupHeaderOffset{ sizeof(Yaz0Header) }; // Skip header

                std::vector<char>& buffer;

                CompressionData(std::vector<char>& buffer) : buffer(buffer) {}
            } compressionData(result);

            result.push_back(0xFFu);

            {
                std::array<uint_least8_t, 8> dummy{};
                size_t dummy_size = dummy.size();
                const int zlibRun = zng_compress2(
                    dummy.data(), &dummy_size, (const uint8_t*)data, dataSize, std::clamp<int>(compressionLevel, 6, 9),
                    [](void* data, uint32_t dist, uint32_t lc) {
                        CompressionData* compressionData = reinterpret_cast<CompressionData*>(data);

                        if (dist == 0) {
                            // Literal.
                            compressionData->groupHeader.set(static_cast<uint8_t>(7 - compressionData->pendingChunks));
                            compressionData->buffer.push_back(static_cast<char>(lc));
                        } else {
                            uint32_t distance = dist - 1;
                            uint32_t length = lc + ZlibMinMatch;

                            if (length < 18) {
                                compressionData->buffer.push_back(static_cast<char>(
                                    ((length - 2) << 4) | static_cast<char>(distance >> 8)
                                ));
                                compressionData->buffer.push_back(static_cast<char>(distance));
                            } else {
                                // If the match is longer than 18 bytes, 3 bytes are needed to write the match.
                                const size_t actual_length = std::min<size_t>(MaximumMatchLength, length);
                                compressionData->buffer.push_back(static_cast<char>(distance >> 8));
                                compressionData->buffer.push_back(static_cast<char>(distance));
                                compressionData->buffer.push_back(static_cast<char>(actual_length - 0x12));
                            }
                        }

                        ++compressionData->pendingChunks;
                        if (compressionData->pendingChunks == ChunksPerGroup) {
                            // Write group header
                            compressionData->buffer[compressionData->groupHeaderOffset] =
                                static_cast<char>(compressionData->groupHeader.to_ulong());
                            
                            // Reset values
                            compressionData->pendingChunks = 0;
                            compressionData->groupHeader.reset();
                            compressionData->groupHeaderOffset = compressionData->buffer.size();
                            compressionData->buffer.push_back(0xFFu);
                        }
                    },
                    &compressionData);

                if (zlibRun != Z_OK) {
                    std::cerr << "[Yaz0::compress] zng_compress failed with code " << std::to_string(zlibRun) << ".\n";
                    return std::nullopt; // return nothing (std::optional)
                }

                if (compressionData.pendingChunks != 0)
                    compressionData.buffer[compressionData.groupHeaderOffset] =
                        static_cast<char>(compressionData.groupHeader.to_ulong());
            }
        }

        return result;
    }

    std::optional<std::vector<char>> decompress(const char* compressedData, const size_t dataSize) {
        const Yaz0Header* header = reinterpret_cast<const Yaz0Header*>(compressedData);

        uint32_t uncompressedSize = BYTESWAP_32(header->uncompressedSize);

        if (std::strncmp(header->magic, "Yaz0", 4) != 0) {
            std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: header magic failed strncmp!\n";
            return std::nullopt; // return nothing (std::optional)
        }

        std::vector<char> destination(uncompressedSize);

        // compressedData without header
        const char* dataSource = compressedData + sizeof(Yaz0Header);

        size_t readOffset{ 0 };
        size_t destOffset{ 0 };

        uint8_t bitsLeft{ 0 };
        uint8_t opcodeByte{ 0 };

        while (destOffset < uncompressedSize) {
            // Read new opcode byte if no bits left
            if (bitsLeft == 0) {
                Common::ReadAtOffset(dataSource, readOffset, dataSize, opcodeByte);

                // Reset bitsLeft (1byte)
                bitsLeft = 8;
            }

            if (readOffset >= dataSize) {
                // We've reached the end of the uncompressed file but we're not finished reading yet;
                // Usually this means the file is invalid.

                std::cerr << "[Yaz0::uncompress] Invalid Yaz0 binary: assertion readOffset >= compressedSize failed!\n";
                return std::nullopt; // return nothing (std::optional)
            }

            if ((opcodeByte & 0x80) != 0) {
                Common::ReadAtOffset(dataSource, readOffset, dataSize, (uint8_t&)destination[destOffset]);
                destOffset++;

                if (readOffset >= (size_t)(compressedData - sizeof(Yaz0Header)))
                    return std::nullopt;
            }
            else {
                uint8_t byteA{ 0 };
                uint8_t byteB{ 0 };

                Common::ReadAtOffset(dataSource, readOffset, dataSize, byteA);
                Common::ReadAtOffset(dataSource, readOffset, dataSize, byteB);

                if (readOffset >= (size_t)(compressedData - sizeof(Yaz0Header)))
                    return std::nullopt;

                uint32_t dist = ((byteA & 0xF) << 8) | byteB;
                size_t copySource = destOffset - (dist + 1);

                uint32_t numBytes = byteA >> 4;

                if (numBytes == 0) {
                    Common::ReadAtOffset(dataSource, readOffset, dataSize, (uint8_t&)numBytes);
                    numBytes += 0x12;
                }
                else
                    numBytes += 2;

                for (uint32_t i = 0; i < numBytes; ++i) {
                    if (destOffset < uncompressedSize)
                        destination[destOffset] = destination[copySource];
                    
                    copySource++;
                    destOffset++;
                }
            }

            opcodeByte <<= 1;
            bitsLeft--;
        }

        return destination;
    }
} // namespace Yaz0