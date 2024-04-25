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

// TODO: test compression

unsigned long compressionSearch(const char* src, uint32_t pos, int max_len, uint32_t range, uint32_t src_end)
{
    unsigned long found_len = 1;
    unsigned long found = 0;

    long search;
    uint32_t cmp_end, cmp1, cmp2;
    char c1;
    uint32_t len;

    if (pos + 2 < src_end)
    {
        search = ((long)pos - (long)range);
        if (search < 0)
            search = 0;

        cmp_end = (uint32_t)(pos + max_len);
        if (cmp_end > src_end)
            cmp_end = src_end;

        c1 = src[pos];
        while (search < pos)
        {
            const char* searchPtr = (const char *)memchr(src, c1, src_end);

            if (!searchPtr)
                break;
            search = searchPtr - src;

            cmp1 = (uint32_t)(search + 1);
            cmp2 = pos + 1;

            while (cmp2 < cmp_end && src[cmp1] == src[cmp2])
            {
                cmp1++; cmp2++;
            }

            len = cmp2 - pos;
            if (found_len < len)
            {
                found_len = len;
                found = (uint32_t)search;
                if ((long)found_len == max_len)
                    break;
            }

            search++;
        }
    }

    return (unsigned long)((found << 32) | found_len);
}

namespace Yaz0 {
    std::vector<char> compress(const char* data, const size_t dataSize, uint8_t compressionLevel = 3) {
        Yaz0Header header;

        strncpy(header.magic, "Yaz0", 4);
        header.uncompressedSize = static_cast<uint32_t>(dataSize);
        header.reserved[0] = 0x0000;
        header.reserved[1] = 0x0000;

        uint32_t range =
            compressionLevel == 0 ?
                0 :
            compressionLevel < 9 ?
                0x10E0 * compressionLevel / 9 - 0x0E0 :
            0x1000;

        size_t readOffset;

        std::vector<char> result(dataSize + (dataSize + 8) / 8);
        size_t destOffset{ sizeof(Yaz0Header) + 1 };
        size_t opcodeOffset{ 0 };

        unsigned long found{ 0 };
        unsigned long foundLen{ 0 };
        uint32_t delta;
        int maxLen{ 0x111 };

        while (readOffset < dataSize) {
            opcodeOffset = destOffset;

            result[destOffset] = 0;
            destOffset++;

            for (uint8_t i = 0; i < 8; i++) {
                if (readOffset >= dataSize)
                    break;
                
                foundLen = 1;

                if (range != 0) {
                    // Going after speed here.
                    // Dunno if using a tuple is slower, so I don't want to risk it.
                    unsigned long search = compressionSearch(data, readOffset, maxLen, range, dataSize);
                    found = search >> 32;
                    foundLen = search & 0xFFFFFFFF;
                }

                if (foundLen > 2) {
                    delta = readOffset - found - 1;

                    if (foundLen < 0x12)
                    {
                        result[destOffset] = delta >> 8 | (foundLen - 2) << 4; destOffset++;
                        result[destOffset] = delta & 0xFF; destOffset++;
                    }
                    else
                    {
                        result[destOffset] = delta >> 8; destOffset++;
                        result[destOffset] = delta & 0xFF; destOffset++;
                        result[destOffset] = (foundLen - 0x12) & 0xFF; destOffset++;
                    }

                    readOffset += foundLen;
                }
                else {
                    result[opcodeOffset] |= 1 << (7 - i);
                    result[destOffset] = data[readOffset]; destOffset++; readOffset++;
                }
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