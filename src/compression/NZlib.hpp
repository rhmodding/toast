#ifndef NZLIB_HPP
#define NZLIB_HPP

#include <vector>

#include <optional>

namespace NZlib {

std::optional<std::vector<unsigned char>> compress(const unsigned char* data, const size_t dataSize, int compressionLevel);

std::optional<std::vector<unsigned char>> decompress(const unsigned char* data, const size_t dataSize);

} // namespace NZlib

#endif // NZLIB_HPP
