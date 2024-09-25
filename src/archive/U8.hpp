#ifndef U8_HPP
#define U8_HPP

#include <vector>

#include <string>

#include <optional>

namespace U8 {

class Directory;

class File {
public:
    std::string name;

    std::vector<unsigned char> data;

    Directory* parent{ nullptr };

    File(const std::string& n);
};

class Directory {
public:
    std::string name;
    std::vector<File> files;
    std::vector<Directory> subdirectories;

    Directory* parent{ nullptr };

    void AddFile(File& file);
    void AddFile(File&& file);

    void AddDirectory(Directory& directory);
    void AddDirectory(Directory&& directory);

    void SortAlphabetically();

    Directory(const std::string& n);

    Directory(const std::string& n, Directory* parentDir);

    Directory* GetParent() const;
};

class U8ArchiveObject {
public:
    Directory structure{ "root" };

    std::vector<unsigned char> Reserialize();

    U8ArchiveObject(const unsigned char* archiveData, const size_t dataSize);

    U8ArchiveObject() {};
};

std::optional<U8ArchiveObject> readYaz0U8Archive(const char* filePath);

std::optional<File> findFile(const char* path, const Directory& directory);

} // namespace U8

#endif // U8_HPP
