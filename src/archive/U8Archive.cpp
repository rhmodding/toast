#include "U8Archive.hpp"

#include <cstdint>
#include <cstring>

#include "../Logging.hpp"

#include <fstream>

#include <utility>
#include <stack>

#include "../compression/Yaz0.hpp"

#include "../common.hpp"

constexpr uint32_t U8_MAGIC = IDENTIFIER_TO_U32('U', 0xAA, '8', '-');

struct U8ArchiveHeader {
    // Compare to U8_MAGIC.
    uint32_t magic { U8_MAGIC };

    // Offset to the node section.
    int32_t nodeSectionStart;
    // Size of the node section (including string pool).
    int32_t nodeSectionSize;

    // Offset to the data section.
    int32_t dataSectionStart;

    // Reserved fields (not used in Fever)
    int32_t _reserved[4] { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
} __attribute__((packed));

struct U8ArchiveNode {
private:
    // Big-endian 32-bit bitfield:
    //     | isDir (8b) | nameOffset (24b) |
    uint32_t isDirAndNameOffset;

public:
    bool isDirectory() const {
        return (this->isDirAndNameOffset & 0xFF) != 0x00;
    }

    // The name offset is relative to the start of the string pool.
    unsigned getNameOffset() const {
        return BYTESWAP_32(this->isDirAndNameOffset & 0xFFFFFF00);
    }

    void setIsDirectory(bool _isDir) {
        this->isDirAndNameOffset &= 0xFFFFFF00;
        if (_isDir)
            this->isDirAndNameOffset |= 1;
    }

    void setNameOffset(unsigned _nameOffset) {
        uint32_t isDir = this->isDirAndNameOffset & 0xFF;
        uint32_t nameOffset = BYTESWAP_32(_nameOffset & 0xFFFFFF);

        this->isDirAndNameOffset = isDir | nameOffset;
    }

public:
    union {
        struct {
            // dataOffset is relative to the start of the file.
            uint32_t dataOffset, dataSize;
        } file;
        struct {
            // parent & nextOutOfDir are both node indices.
            uint32_t parent, nextOutOfDir;
        } dir;
    };
} __attribute__((packed));

namespace U8Archive {

File::File(const std::string& n) :
    name(n)
{}

Directory::Directory(const std::string& n) :
    name(n)
{}
Directory::Directory(const std::string& n, Directory* parentDir) :
    name(n), parent(parentDir)
{}

void Directory::AddFile(File& file) {
    file.parent = this;
    this->files.push_back(file);
}
void Directory::AddFile(File&& file) {
    file.parent = this;
    this->files.push_back(std::move(file));
}

void Directory::AddDirectory(Directory& directory) {
    directory.parent = this;
    this->subdirectories.push_back(directory);
}
void Directory::AddDirectory(Directory&& directory) {
    directory.parent = this;
    this->subdirectories.push_back(std::move(directory));
}

void Directory::SortAlphabetically() {
    this->files.sort([](const File& a, const File& b) {
        return a.name < b.name;
    });
    this->subdirectories.sort([](const Directory& a, const Directory& b) {
        return a.name < b.name;
    });
}

Directory* Directory::GetParent() const {
    return this->parent;
}

U8ArchiveObject::U8ArchiveObject(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(U8ArchiveHeader)) {
        Logging::err << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: data size smaller than header size!" << std::endl;
        return;
    }

    const U8ArchiveHeader* header = reinterpret_cast<const U8ArchiveHeader*>(data);
    if (header->magic != U8_MAGIC) {
        Logging::err << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: header magic failed check!" << std::endl;
        return;
    }

    const int32_t nodeSectionStart = BYTESWAP_32(header->nodeSectionStart);

    const U8ArchiveNode* nodes = reinterpret_cast<const U8ArchiveNode*>(
        data + nodeSectionStart
    );
    const uint32_t nodeCount = BYTESWAP_32(nodes[0].dir.nextOutOfDir);

    const char* stringPool = reinterpret_cast<const char*>(nodes + nodeCount);

    //                   ptr         nextOutOfDir
    std::stack<std::pair<Directory*, unsigned>> dirStack;
    Directory* currentDirectory { &this->structure };

    dirStack.push({ currentDirectory, 0 });

    // Read nodes, except for the first (root node)
    for (unsigned i = 1; i < nodeCount; i++) {
        const U8ArchiveNode* node = nodes + i;

        const char* name = stringPool + node->getNameOffset();

        if (node->isDirectory()) {
            currentDirectory->AddDirectory(Directory(name));

            currentDirectory = &currentDirectory->subdirectories.back();
            dirStack.push({ currentDirectory, BYTESWAP_32(node->dir.nextOutOfDir) });
        }
        else {
            File file(name);

            const unsigned char* fileDataStart = data + BYTESWAP_32(node->file.dataOffset);
            const unsigned char* fileDataEnd   = fileDataStart + BYTESWAP_32(node->file.dataSize);

            file.data = std::vector<unsigned char>(fileDataStart, fileDataEnd);

            currentDirectory->AddFile(std::move(file));
        }

        while (!dirStack.empty() && dirStack.top().second == i + 1) {
            dirStack.pop();
            if (!dirStack.empty())
                currentDirectory = dirStack.top().first;
        }
    }

    this->initialized = true;
}

std::vector<unsigned char> U8ArchiveObject::Serialize() {
    std::vector<unsigned char> result(sizeof(U8ArchiveHeader));
    U8ArchiveHeader* header = reinterpret_cast<U8ArchiveHeader*>(result.data());

    *header = U8ArchiveHeader {
        .nodeSectionStart = BYTESWAP_32(sizeof(U8ArchiveHeader))
    };

    struct FlatEntry {
        union {
            Directory* dir;
            File* file;
        };
        bool isDir;

        unsigned parent;
        unsigned nextOutOfDir;
    };

    // Initialize the root directory entry.
    std::vector<FlatEntry> flattenedArchive = {
        {
            .dir = &this->structure,
            .isDir = true,
            
            .parent = 0,
            // .nextOutOfDir = 0,
            // nextOutOfDir is set later.
        }
    };

    std::stack<std::pair<Directory*, std::list<File>::iterator>> fileItStack;
    std::stack<std::pair<Directory*, std::list<Directory>::iterator>> directoryItStack;

    // Push the root directory.
    fileItStack.push({ &this->structure, this->structure.files.begin() });
    directoryItStack.push({ &this->structure, this->structure.subdirectories.begin() });

    std::vector<unsigned> parentList = { 0 };

    while (!fileItStack.empty()) {
        const Directory* currentDir = fileItStack.top().first;
    
        auto& fileIt = fileItStack.top().second;
        auto& dirIt = directoryItStack.top().second;

        // Process files.
        if (fileIt != currentDir->files.end()) {
            flattenedArchive.push_back({
                .file = &(*fileIt),
                .isDir = false,

                .parent = parentList.back()
                // .nextOutOfDir = 0,
                // nextOutOfDir is not used.
            });
            ++fileIt;
        }
        // Process subdirectories.
        else if (dirIt != currentDir->subdirectories.end()) {
            Directory* subDir = &(*dirIt);
            flattenedArchive.push_back({
                .dir = subDir,
                .isDir = true,

                .parent = parentList.back()
                // .nextOutOfDir = 0,
                // nextOutOfDir is set later.
            });

            parentList.push_back(flattenedArchive.size() - 1);
            ++dirIt;

            fileItStack.push({ subDir, subDir->files.begin() });
            directoryItStack.push({ subDir, subDir->subdirectories.begin() });
        }
        // All files and subdirectories processed; backtrack.
        else {
            for (unsigned parentIndex : parentList)
                flattenedArchive[parentIndex].nextOutOfDir = flattenedArchive.size();

            parentList.pop_back();
            fileItStack.pop();
            directoryItStack.pop();
        }
    }

    std::vector<unsigned> stringOffsets(flattenedArchive.size(), 0);
    std::vector<unsigned> dataOffsets(flattenedArchive.size(), 0);

    unsigned nextStringPoolOffset { 0 };

    // Calculate string offsets.
    for (unsigned i = 0; i < flattenedArchive.size(); ++i) {
        const FlatEntry& entry = flattenedArchive[i];

        stringOffsets[i] = nextStringPoolOffset;
        nextStringPoolOffset += (
            entry.isDir ? entry.dir->name.size() : entry.file->name.size()
        ) + 1;

        nextStringPoolOffset = ALIGN_UP_4(nextStringPoolOffset);
    }

    header->nodeSectionSize = BYTESWAP_32(static_cast<uint32_t>(
        (sizeof(U8ArchiveNode) * flattenedArchive.size()) + nextStringPoolOffset
    ));

    // End of string pool is start of data section.

    unsigned baseDataOffset = ALIGN_UP_32(
        sizeof(U8ArchiveHeader) +
        (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
        nextStringPoolOffset
    );
    unsigned nextDataOffset = baseDataOffset;

    header->dataSectionStart = BYTESWAP_32(baseDataOffset);

    // Calculate data offsets.
    for (unsigned i = 1; i < flattenedArchive.size(); ++i) {
        const FlatEntry& entry = flattenedArchive[i];

        if (!entry.isDir) {
            dataOffsets[i] = nextDataOffset;

            nextDataOffset += entry.file->data.size();
            nextDataOffset = ALIGN_UP_32(nextDataOffset);
        }
    }

    result.resize(nextDataOffset);
    header = reinterpret_cast<U8ArchiveHeader*>(result.data());

    // Write nodes
    for (unsigned i = 0; i < flattenedArchive.size(); ++i) {
        const FlatEntry& entry = flattenedArchive[i];
        U8ArchiveNode* node = reinterpret_cast<U8ArchiveNode*>(
            result.data() + sizeof(U8ArchiveHeader) +
            (sizeof(U8ArchiveNode) * i)
        );

        node->setIsDirectory(entry.isDir);
        node->setNameOffset(stringOffsets[i]);

        char* nameDest = reinterpret_cast<char*>(
            result.data() + sizeof(U8ArchiveHeader) +
            (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
            stringOffsets[i]
        );

        if (entry.isDir) {
            node->dir.nextOutOfDir = BYTESWAP_32(entry.nextOutOfDir);
            node->dir.parent = BYTESWAP_32(entry.parent);

            // Copy name.
            strcpy(nameDest, entry.dir->name.c_str());
        }
        else {
            const unsigned char* fileData = entry.file->data.data();
            unsigned fileDataSize = entry.file->data.size();

            node->file.dataOffset = BYTESWAP_32(dataOffsets[i]);
            node->file.dataSize = BYTESWAP_32(fileDataSize);

            // Copy name.
            strcpy(nameDest, entry.file->name.c_str());
            // Copy data.
            memcpy(result.data() + dataOffsets[i], fileData, fileDataSize);
        }
    }

    return result;
}

std::optional<U8ArchiveObject> readYaz0U8Archive(std::string_view filePath) {
    std::ifstream file(filePath.data(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logging::err << "[U8Archive::readYaz0U8Archive] Error opening file at path: " << filePath << std::endl;
        return std::nullopt;
    }

    const std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    unsigned char* buffer = new unsigned char[fileSize];
    file.read(reinterpret_cast<char*>(buffer), fileSize);

    file.close();

    // yaz0 decompress
    const auto decompressedData = Yaz0::decompress(
        buffer,
        fileSize
    );

    delete[] buffer;

    if (!decompressedData.has_value()) {
        Logging::err << "[U8Archive::readYaz0U8Archive] Error decompressing file at path: " << filePath << std::endl;
        return std::nullopt;
    }

    U8ArchiveObject archive(
        decompressedData->data(),
        decompressedData->size()
    );

    return archive;
}

File* findFile(std::string_view path, Directory& directory) {
    size_t slashOffset = path.find('/');

    // Slash not found: it's a file, search for it
    if (slashOffset == std::string_view::npos) {
        for (File& file : directory.files)
            if (file.name == path)
                return &file;

        return nullptr;
    }
    // Slash found: it's a subdirectory, recursive search
    else {
        for (Directory& subDir : directory.subdirectories)
            if (strncmp(subDir.name.data(), path.data(), slashOffset) == 0)
                return findFile(path.substr(slashOffset + 1), subDir);

        return nullptr;
    }
}

} // namespace U8
