#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <string>
#include <string_view>

#include <vector>

#include <list>

namespace Archive {

class Directory;

class File {
public:
    File(std::string n) :
        name(std::move(n))
    {}

public:
    std::string name;
    std::vector<unsigned char> data;

    Directory* parent { nullptr };
};

class Directory {
public:
    Directory() = default;
    Directory(std::string n) :
        name(std::move(n))
    {}
    Directory(std::string n, Directory* parentDir) :
        name(std::move(n)), parent(parentDir)
    {}

    void AddFile(File& file) {
        file.parent = this;
        this->files.push_back(file);
    }
    void AddFile(File&& file) {
        file.parent = this;
        this->files.push_back(std::move(file));
    }

    void AddDirectory(Directory& directory) {
        directory.parent = this;
        this->subdirectories.push_back(directory);
    }
    void AddDirectory(Directory&& directory) {
        directory.parent = this;
        this->subdirectories.push_back(std::move(directory));
    }

    // Sort all files and subdirectories in alphabetic order.
    void SortAlphabetic();

public:
    std::string name;

    std::list<File> files;
    std::list<Directory> subdirectories;

    Directory* parent { nullptr };
};

class ArchiveObjectBase {
public:
    ArchiveObjectBase() = default;

    bool isInitialized() const { return mInitialized; }

    Directory& getStructure() { return mStructure; }
    const Directory& getStructure() const { return mStructure; }

protected:
    bool mInitialized { false };

    Directory mStructure;
};

const File* findFile(std::string_view path, const Directory& directory);
inline File* findFile(std::string_view path, Directory& directory) {
    return const_cast<File*>(findFile(path, static_cast<const Directory&>(directory)));
}

} // namespace Archive

#endif // ARCHIVE_HPP
