#include "U8.hpp"

#include <cstdint>
#include <cstring>

#include <iostream>
#include <fstream>

#include <algorithm>

#include <utility>
#include <stack>

#include "../compression/Yaz0.hpp"

#include "../common.hpp"

#define HEADER_MAGIC IDENTIFIER_TO_U32('U', 0xAA, '8', '-')

struct U8ArchiveHeader {
    // Magic value (should always equal to 0x55AA382D [1437218861] if valid)
    // Compare to HEADER_MAGIC
    uint32_t magic { HEADER_MAGIC };

    // Offset to the node section, always 0x20
    int32_t nodeSectionStart;

    // Size of node section (section includes string pool)
    int32_t nodeSectionSize;

    // Offset to the data section
    int32_t dataSectionStart;

    // Reserved (not used in Fever)
    int32_t _reserved[4] { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
} __attribute__((packed));

struct U8ArchiveNode {
private:
    uint32_t isDir : 8;
    uint32_t nameOffset : 24;

public:
    bool getIsDir() const {
        return this->isDir != 0x00;
    }

    // Offset is relative to string pool start
    unsigned getNameOffset() const {
        return BYTESWAP_32(this->nameOffset << 8);
    }

    // nameOffset is relative to string pool start
    void setAttributes(bool _isDir, unsigned _nameOffset) {
        this->isDir = _isDir ? 0x01 : 0x00;
        this->nameOffset = BYTESWAP_32(_nameOffset) >> 8;
    }

public:
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

U8ArchiveObject::U8ArchiveObject(const unsigned char* archiveData, const size_t dataSize) {
    if (dataSize < sizeof(U8ArchiveHeader)) {
        std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: data size smaller than header size!\n";
        return;
    }

    const U8ArchiveHeader* header = reinterpret_cast<const U8ArchiveHeader*>(archiveData);
    if (header->magic != HEADER_MAGIC) {
        std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: header magic failed check!\n";
        return;
    }

    const int32_t nodeSectionStart = BYTESWAP_32(header->nodeSectionStart);

    const U8ArchiveNode* nodes = reinterpret_cast<const U8ArchiveNode*>(
        archiveData + nodeSectionStart
    );
    const uint32_t nodeCount = BYTESWAP_32(nodes[0].sizeOrNextEntry);

    const char* stringPool = reinterpret_cast<const char*>(nodes + nodeCount);

    //                   ptr         nextEntry
    std::stack<std::pair<Directory*, unsigned>> dirStack;
    Directory* currentDirectory { &this->structure };

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
        }
        else {
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

    *header = U8ArchiveHeader {
        .nodeSectionStart = BYTESWAP_32(sizeof(U8ArchiveHeader))
    };

    struct FlatEntry {
        void* ptr;
        unsigned parent;
        unsigned nextOutOfDir;
        bool isDir;
    };
    std::vector<FlatEntry> flattenedArchive = {
        { &this->structure, 0, 0, true } // Root node
    };

    std::stack<std::pair<Directory*, std::list<File>::iterator>> fileItStack;
    std::stack<std::pair<Directory*, std::list<Directory>::iterator>> directoryItStack;

    // Start with the root directory
    fileItStack.push({ &this->structure, this->structure.files.begin() });
    directoryItStack.push({ &this->structure, this->structure.subdirectories.begin() });

    std::vector<unsigned> parentList = { 0 };

    while (!fileItStack.empty()) {
        Directory* currentDir = fileItStack.top().first;
        auto& fileIt = fileItStack.top().second;
        auto& dirIt = directoryItStack.top().second;

        // Process files
        if (fileIt != currentDir->files.end()) {
            flattenedArchive.push_back({
                .ptr = &(*fileIt), 
                .parent = parentList.back(), 
                // .nextOutOfDir = 0, 
                // nextOutOfDir is not used.
                .isDir = false
            });
            ++fileIt;
        }
        // Process subdirectories
        else if (dirIt != currentDir->subdirectories.end()) {
            Directory* subDir = &(*dirIt);
            flattenedArchive.push_back({
                .ptr = subDir, 
                .parent = parentList.back(), 
                // .nextOutOfDir = 0,
                // nextOutOfDir is set later.
                .isDir = true
            });

            parentList.push_back(flattenedArchive.size() - 1);
            ++dirIt;

            fileItStack.push({ subDir, subDir->files.begin() });
            directoryItStack.push({ subDir, subDir->subdirectories.begin() });
        }
        // All files and subdirectories processed, backtrack
        else {
            for (unsigned parentIndex : parentList)
                flattenedArchive[parentIndex].nextOutOfDir = flattenedArchive.size();

            parentList.pop_back();
            fileItStack.pop();
            directoryItStack.pop();
        }
    }

    unsigned stringPoolSize { 0 };
    unsigned dataSize { 0 };

    std::vector<unsigned> stringOffsets(flattenedArchive.size(), 0);
    std::vector<unsigned> dataOffsets(flattenedArchive.size(), 0);

    // Calculate string offsets
    for (unsigned i = 0; i < flattenedArchive.size(); ++i) {
        const FlatEntry& entry = flattenedArchive[i];

        stringOffsets[i] = stringPoolSize;
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
            unsigned char* fileData = reinterpret_cast<File*>(entry.ptr)->data.data();
            unsigned fileDataSize = reinterpret_cast<File*>(entry.ptr)->data.size();

            node->setAttributes(false, stringOffsets[i]);
            node->sizeOrNextEntry = BYTESWAP_32(fileDataSize);
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
            memcpy(result.data() + dataOffsets[i], fileData, fileDataSize);
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
