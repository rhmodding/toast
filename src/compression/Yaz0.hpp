#ifndef YAZ0_HPP
#define YAZ0_HPP

#include <vector>
#include <optional>

#include <cstdint>

namespace Yaz0 {
    std::optional<std::vector<unsigned char>> decompress(const unsigned char* compressedData, const size_t dataSize);

    std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, uint8_t compressionLevel = 7);
}

#endif // YAZ0_HPP