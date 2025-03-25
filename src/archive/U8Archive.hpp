#ifndef U8_HPP
#define U8_HPP

#include <string>

#include <vector>

#include <list>

#include <optional>

#include <string_view>

// Although commonly referred to as a "U8 archive", it's true name is DARCH (meaning
// DVD archive). However, the term "U8 archive" has become widely adopted, while the
// correct name DARCH remains pretty much unknown (try googling U8 archive vs DARCH, lol).
//
// For clarity reasons, we use the term "U8 archive" throughout toast.

namespace U8Archive {

class Directory;

class File {
public:
    std::string name;

    std::vector<unsigned char> data;

    Directory* parent { nullptr };

    File(const std::string& n);
};

class Directory {
public:
    std::string name;
    std::list<File> files;
    std::list<Directory> subdirectories;

    Directory* parent { nullptr };

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
    U8ArchiveObject(const unsigned char* data, const size_t dataSize);
    U8ArchiveObject() = default;

    bool isInitialized() const { return this->initialized; }

    std::vector<unsigned char> Serialize();

public:
    Directory structure { "" };

private:
    bool initialized { false };
};

std::optional<U8ArchiveObject> readYaz0U8Archive(std::string_view filePath);

File* findFile(std::string_view path, Directory& directory);

} // namespace U8

#endif // U8_HPP
