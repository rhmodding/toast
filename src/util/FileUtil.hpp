#ifndef FILE_UTIL_HPP
#define FILE_UTIL_HPP

#include <string_view>

#include <optional>

#include <vector>

namespace FileUtil {

// Open a binary from the filesystem and read it's data.
//
// Returns: std::vector<unsigned char> wrapped in std::optional
std::optional<std::vector<unsigned char>> openFileData(std::string_view filePath);

bool doesFileExist(std::string_view filePath);

// Copy one file to another by their paths. If overwrite is set, 
//
// Returns: true if successfully copied (or file already exists and overwrite is set), false if failed
bool copyFile(std::string_view filePathSrc, std::string_view filePathDst, bool overwrite);

} // namespace FileUtil

#endif // FILE_UTIL_HPP
