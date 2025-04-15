#ifndef SARC_HPP
#define SARC_HPP

#include <string>

#include <vector>

#include <list>

#include <optional>

#include <string_view>

namespace SARC {

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

    Directory(const std::string& n);
    Directory(const std::string& n, Directory* parentDir);

    Directory* GetParent() const;
};

class SARCObject {
public:
    SARCObject(const unsigned char* data, const size_t dataSize);
    SARCObject() = default;

    bool isInitialized() const { return this->initialized; }

    [[nodiscard]] std::vector<unsigned char> Serialize();

public:
    Directory structure { "" };

private:
    bool initialized { false };
};

[[nodiscard]] std::optional<SARCObject> readNZlibSARC(std::string_view filePath);

File* findFile(std::string_view path, Directory& directory);

} // namespace SARC

#endif // SARC_HPP
