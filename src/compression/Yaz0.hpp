#ifndef YAZ0_HPP
#define YAZ0_HPP

#include <cstddef>

#include <vector>

#include <optional>

namespace Yaz0 {

[[nodiscard]] std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel);

[[nodiscard]] std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize);

bool checkDataValid(const unsigned char* data, const size_t dataSize);

} // namespace Yaz0

#endif // YAZ0_HPP
