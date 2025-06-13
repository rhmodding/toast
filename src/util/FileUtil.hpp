#ifndef FILE_UTIL_HPP
#define FILE_UTIL_HPP

#include <string_view>

namespace FileUtil {

bool doesFileExist(std::string_view filePath);

// Copy one file to another by their paths. If overwrite is set, 
//
// Returns: true if successfully copied (or file already exists and overwrite is set), false if failed
bool copyFile(std::string_view filePathSrc, std::string_view filePathDst, bool overwrite);

} // namespace FileUtil

#endif // FILE_UTIL_HPP
