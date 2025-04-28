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
    File(std::string n);

public:
    std::string name;

    std::vector<unsigned char> data;

    Directory* parent { nullptr };
};

class Directory {
public:
    Directory(std::string n);
    Directory(std::string n, Directory* parentDir);

    void AddFile(File& file);
    void AddFile(File&& file);

    void AddDirectory(Directory& directory);
    void AddDirectory(Directory&& directory);

    void SortAlphabetically();

public:
    std::string name;

    // We use a linked list since files and directories hold pointers to their parent.
    std::list<File> files;
    std::list<Directory> subdirectories;

    Directory* parent { nullptr };
};

class U8ArchiveObject {
public:
    U8ArchiveObject(const unsigned char* data, const size_t dataSize);
    U8ArchiveObject() = default;

    bool isInitialized() const { return this->initialized; }

    [[nodiscard]] std::vector<unsigned char> Serialize();

public:
    Directory structure { "" };

private:
    bool initialized { false };
};

[[nodiscard]] std::optional<U8ArchiveObject> readYaz0U8Archive(std::string_view filePath);

const File* findFile(std::string_view path, const Directory& directory);
inline File* findFile(std::string_view path, Directory& directory) {
    return const_cast<File*>(findFile(path, static_cast<const Directory&>(directory)));
}

} // namespace U8

#endif // U8_HPP
