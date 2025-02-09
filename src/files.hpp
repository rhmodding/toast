#ifndef FILES_HPP
#define FILES_HPP

namespace Files {

bool doesFileExist(const char* filePath);

// Backup a file at the specified path. If once is true, avoid writing the
// backup file if it already exists.
//
// Returns: true if succeeded, false if failed
bool BackupFile(const char* filePath, bool once);

} // namespace Files

#endif // FILES_HPP
