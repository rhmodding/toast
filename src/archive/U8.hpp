#ifndef U8_HPP
#define U8_HPP

#include <string>
#include <vector>
#include <optional>

namespace U8 {
    class Directory;

    // Note: storing a Directory or File directly is very expensive! Consider storing a pointer or reference instead.
    class File {
    public:
        std::string name;

        std::vector<char> data;

        Directory* parent{ nullptr };

        File(const std::string& n);
    };

    // Note: storing a Directory or File directly is very expensive! Consider storing a pointer or reference instead.
    class Directory {
    public:
        std::string name;
        std::vector<File> files;
        std::vector<Directory> subdirectories;

        // So we don't have to re-calculate for re-serialization.
        uint32_t totalNodeCount;

        Directory* parent{ nullptr };

        void AddFile(File& file);
        void AddDirectory(Directory& directory);

        Directory(const std::string& n);

        Directory(const std::string& n, Directory* parentDir);

        Directory* GetParent() const;
    };

    class U8ArchiveObject {
    public:
        Directory structure{ "root" };

        std::vector<char> Reserialize();

        U8ArchiveObject(const char* archiveData, const size_t dataSize);
    };

    std::optional<U8ArchiveObject> readYaz0U8Archive(const std::string& filePath);

    std::optional<File> findFile(const std::string& path, const Directory& directory);
}

#endif // U8_HPP