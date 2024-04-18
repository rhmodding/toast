#include "Yaz0.hpp"

#include "../common.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <optional>
#include <cstdint>

#pragma pack(push, 1) // Pack struct members tightly without padding

struct Yaz0Header {
    // Magic string (should always equal to "Yaz0" if valid)
    char magic[4];

    // Size of the file before compression in bytes
    uint32_t uncompressedSize;

    // Reserved bytes. These are always present in the header
    uint32_t reserved[2];
};

#pragma pack(pop) // Reset packing alignment to default

namespace Yaz0 {
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

        while (destOffset <= uncompressedSize) {
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