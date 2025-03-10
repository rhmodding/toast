#ifndef FILES_HPP
#define FILES_HPP

namespace Files {

bool doesFileExist(const char* filePath);

// Copy one file to another by their paths. If overwrite is set, 
//
// Returns: true if successfully copied (or file already exists and overwrite is set), false if failed
bool copyFile(const char* filePathSrc, const char* filePathDst, bool overwrite);

} // namespace Files

#endif // FILES_HPP
