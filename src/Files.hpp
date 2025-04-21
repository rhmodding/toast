#ifndef FILES_HPP
#define FILES_HPP

#include <string_view>

namespace Files {

bool doesFileExist(std::string_view filePath);

// Copy one file to another by their paths. If overwrite is set, 
//
// Returns: true if successfully copied (or file already exists and overwrite is set), false if failed
bool copyFile(std::string_view filePathSrc, std::string_view filePathDst, bool overwrite);

} // namespace Files

#endif // FILES_HPP
