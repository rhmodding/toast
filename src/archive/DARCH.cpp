#include "DARCH.hpp"

#include <cstdint>
#include <cstring>

#include "Logging.hpp"

#include <fstream>

#include <utility>
#include <stack>

#include "compression/Yaz0.hpp"

#include "Macro.hpp"

constexpr uint32_t DARCH_MAGIC = IDENTIFIER_TO_U32('U',0xAAu,'8','-');

struct DARCHHeader {
    // Compare to DARCH_MAGIC.
    uint32_t magic { DARCH_MAGIC };

    // Offset to the node section.
    int32_t nodeSectionStart;
    // Size of the node section (including string pool).
    int32_t nodeSectionSize;

    // Offset to the data section.
    int32_t dataSectionStart;

    // Reserved fields (not used in Fever)
    int32_t _reserved[4] { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
} __attribute__((packed));

struct DARCHNode {
private:
    // Big-endian 32-bit bitfield:
    //     | isDir (8b) | nameOffset (24b) |
    uint32_t isDirAndNameOffset;

public:
    bool isDirectory() const {
        return (this->isDirAndNameOffset & 0x000000FF) != 0x00;
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
        uint32_t isDir = (this->isDirAndNameOffset & 0x000000FF);
        uint32_t nameOffset = BYTESWAP_32(_nameOffset & 0x00FFFFFF);

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

namespace Archive {

DARCHObject::DARCHObject(const unsigned char *data, const size_t dataSize) {
    if (dataSize < sizeof(DARCHHeader)) {
        Logging::error("[DARCHObject::DARCHObject] Invalid DARCH binary: data size smaller than header size!");
        return;
    }

    const DARCHHeader *header = reinterpret_cast<const DARCHHeader*>(data);
    if (header->magic != DARCH_MAGIC) {
        Logging::error("[DARCHObject::DARCHObject] Invalid DARCH binary: header magic failed check!");
        return;
    }

    const int32_t nodeSectionStart = BYTESWAP_32(header->nodeSectionStart);

    const DARCHNode *nodes = reinterpret_cast<const DARCHNode*>(
        data + nodeSectionStart
    );
    const uint32_t nodeCount = BYTESWAP_32(nodes[0].dir.nextOutOfDir);

    const char *stringPool = reinterpret_cast<const char*>(nodes + nodeCount);

    //                   directory             nextOutOfDir
    std::stack<std::pair<Archive::Directory *, unsigned>> dirStack;
    Archive::Directory* currentDirectory { &mStructure };

    dirStack.push({ currentDirectory, 0 });

    // Read nodes, except for the first (root node)
    for (unsigned i = 1; i < nodeCount; i++) {
        const DARCHNode *node = nodes + i;

        const char *name = stringPool + node->getNameOffset();

        if (node->isDirectory()) {
            currentDirectory->addDirectory(Archive::Directory(name));

            currentDirectory = &currentDirectory->subdirectories.back();
            dirStack.push({ currentDirectory, BYTESWAP_32(node->dir.nextOutOfDir) });
        }
        else {
            Archive::File file(name);

            const unsigned char* fileDataStart = data + BYTESWAP_32(node->file.dataOffset);
            const unsigned char* fileDataEnd   = fileDataStart + BYTESWAP_32(node->file.dataSize);

            file.data = std::vector<unsigned char>(fileDataStart, fileDataEnd);

            currentDirectory->addFile(std::move(file));
        }

        while (!dirStack.empty() && dirStack.top().second == i + 1) {
            dirStack.pop();
            if (!dirStack.empty()) {
                currentDirectory = dirStack.top().first;
            }
        }
    }

    mInitialized = true;
}

std::vector<unsigned char> DARCHObject::serialize() {
    std::vector<unsigned char> result(sizeof(DARCHHeader));

    DARCHHeader *header = reinterpret_cast<DARCHHeader*>(result.data());
    *header = DARCHHeader {
        .nodeSectionStart = BYTESWAP_32(sizeof(DARCHHeader))
    };

    struct FlatEntry {
        union {
            Archive::Directory *dir;
            Archive::File *file;
        };
        bool isDir;

        unsigned parent;
        unsigned nextOutOfDir;
    };

    // Initialize the root directory entry.
    std::vector<FlatEntry> flattenedArchive = {
        {
            .dir = &mStructure,
            .isDir = true,

            .parent = 0,
            // .nextOutOfDir = 0,
            // nextOutOfDir is set later.
        }
    };

    std::stack<std::pair<Archive::Directory *, std::list<Archive::File>::iterator>> fileItStack;
    std::stack<std::pair<Archive::Directory *, std::list<Archive::Directory>::iterator>> directoryItStack;

    // Push the root directory.
    fileItStack.push({ &mStructure, mStructure.files.begin() });
    directoryItStack.push({ &mStructure, mStructure.subdirectories.begin() });

    std::vector<unsigned> parentList = { 0 };

    while (!fileItStack.empty()) {
        const Archive::Directory* currentDir = fileItStack.top().first;

        auto &fileIt = fileItStack.top().second;
        auto &dirIt = directoryItStack.top().second;

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
            Archive::Directory* subDir = &(*dirIt);
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
            for (unsigned parentIndex : parentList) {
                flattenedArchive[parentIndex].nextOutOfDir = flattenedArchive.size();
            }

            parentList.pop_back();
            fileItStack.pop();
            directoryItStack.pop();
        }
    }

    std::vector<unsigned> stringOffsets(flattenedArchive.size(), 0);
    std::vector<unsigned> dataOffsets(flattenedArchive.size(), 0);

    unsigned nextStringPoolOffset { 0 };

    // Calculate string offsets.
    for (size_t i = 0; i < flattenedArchive.size(); ++i) {
        const FlatEntry &entry = flattenedArchive[i];

        stringOffsets[i] = nextStringPoolOffset;
        nextStringPoolOffset += (
            entry.isDir ? entry.dir->name.size() : entry.file->name.size()
        ) + 1;
    }

    header->nodeSectionSize = BYTESWAP_32(static_cast<uint32_t>(
        (sizeof(DARCHNode) * flattenedArchive.size()) + nextStringPoolOffset
    ));

    // End of string pool is start of data section.

    unsigned baseDataOffset = ALIGN_UP_32(
        sizeof(DARCHHeader) +
        (sizeof(DARCHNode) * flattenedArchive.size()) +
        nextStringPoolOffset
    );
    unsigned nextDataOffset = baseDataOffset;

    header->dataSectionStart = BYTESWAP_32(baseDataOffset);

    // Calculate data offsets.
    for (size_t i = 1; i < flattenedArchive.size(); ++i) {
        const FlatEntry &entry = flattenedArchive[i];

        if (!entry.isDir) {
            dataOffsets[i] = nextDataOffset;

            nextDataOffset += entry.file->data.size();
            if (i != (flattenedArchive.size() - 1))
                nextDataOffset = ALIGN_UP_32(nextDataOffset);
        }
    }

    result.resize(nextDataOffset);
    header = reinterpret_cast<DARCHHeader *>(result.data());

    // Write nodes.
    for (size_t i = 0; i < flattenedArchive.size(); ++i) {
        const FlatEntry &entry = flattenedArchive[i];
        DARCHNode *node = reinterpret_cast<DARCHNode*>(
            result.data() + sizeof(DARCHHeader) +
            (sizeof(DARCHNode) * i)
        );

        node->setIsDirectory(entry.isDir);
        node->setNameOffset(stringOffsets[i]);

        char *nameDest = reinterpret_cast<char *>(
            result.data() + sizeof(DARCHHeader) +
            (sizeof(DARCHNode) * flattenedArchive.size()) +
            stringOffsets[i]
        );

        if (entry.isDir) {
            node->dir.nextOutOfDir = BYTESWAP_32(entry.nextOutOfDir);
            node->dir.parent = BYTESWAP_32(entry.parent);

            // Copy name.
            std::strcpy(nameDest, entry.dir->name.c_str());
        }
        else {
            const unsigned char *fileData = entry.file->data.data();
            unsigned fileDataSize = entry.file->data.size();

            node->file.dataOffset = BYTESWAP_32(dataOffsets[i]);
            node->file.dataSize = BYTESWAP_32(fileDataSize);

            // Copy name.
            std::strcpy(nameDest, entry.file->name.c_str());
            // Copy data.
            std::memcpy(result.data() + dataOffsets[i], fileData, fileDataSize);
        }
    }

    return result;
}

} // namespace Archive
