#include "U8.hpp"

#include <cstdint>
#include <cstring>

#include <iostream>
#include <fstream>

#include <algorithm>

#include <vector>

#include <utility>
#include <stack>

#include "../compression/Yaz0.hpp"

#include "../common.hpp"

#define HEADER_MAGIC IDENTIFIER_TO_U32('U', 0xAA, '8', '-')

struct U8ArchiveHeader {
    // Magic value (should always equal to 0x55AA382D [1437218861] if valid)
    uint32_t magic;

    // Offset to the node section, always 0x20
    int32_t nodeSectionStart;

    // Size of node section (section includes string pool)
    int32_t nodeSectionSize;

    // Offset to the data section
    int32_t dataSectionStart;

    // Reserved (not used in Fever)
    int32_t reserved[4];
} __attribute__((packed));

struct U8ArchiveNode {
    uint32_t attributes{ 0x00000000 }; // | isDir (1b) |    name offset (3b)   |

    // 0x00: file, 0x01: directory
    inline bool getIsDir() const {
        return (this->attributes & 0x000000FF) == 0 ? false : true;
    }

    // Offset into the string pool for the name of the node.
    // Returns LE value
    inline unsigned getNameOffset() const {
        return BYTESWAP_32(this->attributes >> 8 << 8);
    }

    // Offset must be LE
    inline void setAttributes(bool isDir, unsigned offset) {
        this->attributes = (BYTESWAP_32(offset) & 0xFFFFFF00) | static_cast<uint8_t>(isDir);
    }

    // File: Offset of begin of data
    // Directory: Index of the parent directory
    uint32_t dataOffsetOrParent;

    // File: Size of data
    // Directory: Index of the first node that is not part of this directory
    uint32_t sizeOrNextEntry;
} __attribute__((packed));

namespace U8 {

File::File(const std::string& n) : name(n) {}

Directory::Directory(const std::string& n) : name(n) {}
Directory::Directory(const std::string& n, Directory* parentDir) : name(n), parent(parentDir) {}

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
    std::sort(this->files.begin(), this->files.end(), [](const File& a, const File& b) {
        return a.name < b.name;
    });

    std::sort(this->subdirectories.begin(), this->subdirectories.end(), [](const Directory& a, const Directory& b) {
        return a.name < b.name;
    });
}

Directory* Directory::GetParent() const {
    return this->parent;
}

U8ArchiveObject::U8ArchiveObject(const unsigned char* archiveData, const size_t dataSize) {
    if (dataSize < sizeof(U8ArchiveHeader)) {
        std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: data size smaller than header size!\n";
        return;
    }

    const U8ArchiveHeader* header = reinterpret_cast<const U8ArchiveHeader*>(archiveData);
    if (UNLIKELY(header->magic != HEADER_MAGIC)) {
        std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: header magic failed check!\n";
        return;
    }

    const int32_t nodeSectionStart = BYTESWAP_32(header->nodeSectionStart);

    const U8ArchiveNode* nodes = reinterpret_cast<const U8ArchiveNode*>(
        archiveData + nodeSectionStart
    );
    const uint32_t nodeCount = BYTESWAP_32(nodes[0].sizeOrNextEntry);

    const char* stringPool = reinterpret_cast<const char*>(nodes + nodeCount);

    std::stack<std::pair<Directory*, unsigned>> dirStack;
    Directory* currentDirectory{ &this->structure };

    dirStack.push({ currentDirectory, 0 });

    // Read nodes, except for the first (root node)
    for (unsigned i = 1; i < nodeCount; i++) {
        const U8ArchiveNode* node = nodes + i;

        const char* name = stringPool + node->getNameOffset();

        if (node->getIsDir()) {
            Directory directory(name);
            currentDirectory->AddDirectory(directory);

            currentDirectory = &currentDirectory->subdirectories.back();
            dirStack.push({currentDirectory, BYTESWAP_32(node->sizeOrNextEntry)});
        } else {
            File file(name);

            const unsigned char* dataStart = archiveData + BYTESWAP_32(node->dataOffsetOrParent);
            file.data = std::vector<unsigned char>(dataStart, dataStart + BYTESWAP_32(node->sizeOrNextEntry));

            currentDirectory->AddFile(file);
        }

        while (!dirStack.empty() && dirStack.top().second == i + 1) {
            dirStack.pop();
            if (!dirStack.empty())
                currentDirectory = dirStack.top().first;
        }
    }

    return;
}

std::vector<unsigned char> U8ArchiveObject::Reserialize() {
    std::vector<unsigned char> result(sizeof(U8ArchiveHeader));
    U8ArchiveHeader* header = reinterpret_cast<U8ArchiveHeader*>(result.data());

    header->magic = HEADER_MAGIC;
    header->nodeSectionStart = BYTESWAP_32(sizeof(U8ArchiveHeader));
    memset(header->reserved, 0x00, sizeof(header->reserved));

    struct FlatEntry {
        void* ptr;
        unsigned parent;
        unsigned nextOutOfDir;
        bool isDir;
    };
    std::vector<FlatEntry> flattenedArchive = {
        { nullptr, 0, 0, true } // Root node
    };

    std::stack<std::pair<Directory*, unsigned>> directoryStack;
    directoryStack.push({ &this->structure, 0 });

    std::vector<unsigned> parentList = { 0 };

    while (!directoryStack.empty()) {
        Directory* currentDir = directoryStack.top().first;
        unsigned& index = directoryStack.top().second;

        if (index < currentDir->files.size()) {
            flattenedArchive.push_back({
                .ptr = &currentDir->files[index++], 
                .parent = parentList.back(), 
                // .nextOutOfDir = 0, 
                // nextOutOfDir is not used.
                .isDir = false
            });
        } 
        else if (index < currentDir->files.size() + currentDir->subdirectories.size()) {
            index -= currentDir->files.size();
            Directory* subDir = &currentDir->subdirectories[index++];
            
            flattenedArchive.push_back({
                .ptr = subDir, 
                .parent = parentList.back(), 
                // .nextOutOfDir = 0,
                // nextOutOfDir is set later.
                .isDir = true
            });

            parentList.push_back(flattenedArchive.size() - 1);
            directoryStack.push({ subDir, 0 });
        } 
        else {
            for (unsigned parentIndex : parentList)
                flattenedArchive[parentIndex].nextOutOfDir = flattenedArchive.size();

            parentList.pop_back();
            directoryStack.pop();
        }
    }

    unsigned stringPoolSize { 1 }; // Root node string's null terminator
    unsigned dataSize { 0 };

    std::vector<unsigned> stringOffsets(1, 0);
    std::vector<unsigned> dataOffsets(flattenedArchive.size(), 0);

    // Calculate string offsets
    for (unsigned i = 1; i < flattenedArchive.size(); ++i) {
        const FlatEntry& entry = flattenedArchive[i];

        stringOffsets.push_back(stringPoolSize);
        stringPoolSize += (entry.isDir ?
            reinterpret_cast<Directory*>(entry.ptr)->name.size() :
            reinterpret_cast<File*>(entry.ptr)->name.size()
        ) + 1;
    }

    // Calculate data offset
    unsigned baseDataOffset = (
        sizeof(U8ArchiveHeader) +
        (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
        stringPoolSize + 63
    ) & ~63;

    // Calculate data offsets
    for (unsigned i = 1; i < flattenedArchive.size(); ++i) {
        const FlatEntry& entry = flattenedArchive[i];

        if (!entry.isDir) {
            dataOffsets[i] = baseDataOffset + dataSize;
            dataSize += reinterpret_cast<File*>(entry.ptr)->data.size();

            if (i + 1 != flattenedArchive.size())
                dataSize = (dataSize + 31) & ~31;
        }
    }

    result.resize(baseDataOffset + dataSize);
    header = reinterpret_cast<U8ArchiveHeader*>(result.data());

    // Set header offsets
    header->dataSectionStart = BYTESWAP_32(baseDataOffset);
    header->nodeSectionSize = BYTESWAP_32(static_cast<uint32_t>(
        (sizeof(U8ArchiveNode) * flattenedArchive.size()) + stringPoolSize
    ));

    // Write nodes
    for (unsigned i = 0; i < flattenedArchive.size(); ++i) {
        FlatEntry& entry = flattenedArchive[i];
        U8ArchiveNode* node = reinterpret_cast<U8ArchiveNode*>(
            result.data() + sizeof(U8ArchiveHeader) +
            (sizeof(U8ArchiveNode) * i)
        );

        if (entry.isDir) {
            node->setAttributes(true, stringOffsets[i]);
            node->sizeOrNextEntry = BYTESWAP_32(entry.nextOutOfDir);
            node->dataOffsetOrParent = BYTESWAP_32(entry.parent);

            // Copy directory name
            if (entry.ptr)
                strcpy(
                    reinterpret_cast<char*>(
                        result.data() + sizeof(U8ArchiveHeader) +
                        (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
                        stringOffsets[i]
                    ),
                    reinterpret_cast<Directory*>(entry.ptr)->name.c_str()
                );
        } 
        else {
            unsigned char* data = reinterpret_cast<File*>(entry.ptr)->data.data();
            unsigned dataSize = reinterpret_cast<File*>(entry.ptr)->data.size();

            node->setAttributes(0x00, stringOffsets[i]);
            node->sizeOrNextEntry = BYTESWAP_32(dataSize);
            node->dataOffsetOrParent = BYTESWAP_32(dataOffsets[i]);

            // Copy file name
            strcpy(
                reinterpret_cast<char*>(
                    result.data() + sizeof(U8ArchiveHeader) +
                    (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
                    stringOffsets[i]
                ),
                reinterpret_cast<File*>(entry.ptr)->name.c_str()
            );
            // Copy file data
            memcpy(result.data() + dataOffsets[i], data, dataSize);
        }
    }

    return result;
}

std::optional<U8ArchiveObject> readYaz0U8Archive(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[U8::readYaz0U8Archive] Error opening file at path: " << filePath << '\n';
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
        std::cerr << "[U8::readYaz0U8Archive] Error decompressing file at path: " << filePath << '\n';
        return std::nullopt;
    }

    U8::U8ArchiveObject archive(
        decompressedData->data(),
        decompressedData->size()
    );

    return archive;
}

File* findFile(const char* path, Directory& directory) {
    const char* slash = strchr(path, '/');

    if (!slash) { // Slash not found: it's a file, search for it
        for (File& file : directory.files)
            if (strcmp(file.name.c_str(), path) == 0)
                return &file;

        return nullptr;
    }
    else { // Slash found, it's a subdirectory, recursive search
        unsigned slashOffset = (slash - path);
        for (Directory& subDir : directory.subdirectories)
            if (strncmp(subDir.name.c_str(), path, slashOffset) == 0)
                return findFile(slash + 1, subDir);

        return nullptr;
    }
}

} // namespace U8
