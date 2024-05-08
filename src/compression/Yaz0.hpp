#ifndef YAZ0_HPP
#define YAZ0_HPP

#include <vector>
#include <optional>

#include <cstdint>

namespace Yaz0 {
    std::optional<std::vector<char>> decompress(const char* compressedData, const size_t dataSize);

    std::optional<std::vector<char>> compress(const char* data, const size_t dataSize, uint8_t compressionLevel = 7);
}

#endif // YAZ0_HPP