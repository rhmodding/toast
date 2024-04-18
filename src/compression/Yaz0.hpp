#ifndef YAZ0_HPP
#define YAZ0_HPP

#include <vector>
#include <optional>

namespace Yaz0 {
    std::optional<std::vector<char>> decompress(const char* compressedData, const size_t dataSize);
}

#endif // YAZ0_HPP