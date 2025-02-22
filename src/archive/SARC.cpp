#include "SARC.hpp"

#include <cstdint>

#include <iostream>

#include <fstream>

#include <sstream>

#include "../compression/NZlib.hpp"

#include "../common.hpp"

constexpr uint32_t SARC_MAGIC = IDENTIFIER_TO_U32('S','A','R','C');
constexpr uint32_t SFAT_MAGIC = IDENTIFIER_TO_U32('S','F','A','T');
constexpr uint32_t SFNT_MAGIC = IDENTIFIER_TO_U32('S','F','N','T');

constexpr uint16_t SARC_VERSION = 0x0100;

constexpr uint16_t SARC_BYTEORDER_BIG    = 0xFFFE;
constexpr uint16_t SARC_BYTEORDER_LITTLE = 0xFEFF;

constexpr uint16_t SARC_DEFAULT_HASH_KEY = 0x65;

constexpr uint16_t SARC_DATA_ALIGN = 0x20;
constexpr uint16_t SARC_NAME_ALIGN = 4;

struct SarcFileHeader {
    uint32_t magic; // Compare to SARC_MAGIC
    uint16_t sectionSize;

    uint16_t byteOrder; // Compare to SARC_BYTEORDER_BIG and SARC_BYTEORDER_LITTLE

    uint32_t fileSize; // Size of this file.
    uint32_t dataStart; // File offset to the archive data.

    uint16_t formatVersion; // Compare to SARC_VERSION

    uint16_t _reserved;
} __attribute((packed));

struct SfatNode {
    uint32_t nameHash;

    struct {
        // In 4byte increments; multiply by 4 to get the true offset.
        // Relative to the start of the SFNT section's data.
        uint32_t nameOffset : 24;

        uint32_t collisionCount : 8;
    };

    uint32_t dataOffsetStart; // Relative to the file header's dataStart.
    uint32_t dataOffsetEnd; // Relative to the file header's dataStart.
} __attribute((packed));

struct SfatSection {
    uint32_t magic; // Compare to SFAT_MAGIC
    uint16_t sectionSize;

    uint16_t nodeCount;

    // SARC_DEFAULT_HASH_KEY by default; can differ in the case of
    // a collision.
    uint32_t hashKey;

    SfatNode nodes[0];
} __attribute((packed));

struct SfntSection {
    uint32_t magic; // Compare to SFNT_MAGIC
    uint16_t sectionSize;

    uint16_t _pad16;

    char data[0];
} __attribute((packed));

namespace SARC {

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

SARCObject::SARCObject(const unsigned char* data, const size_t dataSize) {
    if (dataSize < sizeof(SarcFileHeader)) {
        std::cerr << "[SARCObject::SARCObject] Invalid SARC binary: data size smaller than header size!\n";
        return;
    }
    
    const SarcFileHeader* header = reinterpret_cast<const SarcFileHeader*>(data);
    if (header->magic != SARC_MAGIC) {
        std::cerr << "[SARCObject::SARCObject] Invalid SARC binary: header magic is nonmatching!\n";
        return;
    }

    if (header->byteOrder == SARC_BYTEORDER_BIG) {
        std::cerr << "[SARCObject::SARCObject] Big-endian SARC is not supported!\n";
        return;
    }
    if (header->byteOrder != SARC_BYTEORDER_LITTLE) {
        std::cerr << "[SARCObject::SARCObject] Invalid SARC binary: byte order mark is invalid!\n";
        return;
    }

    if (header->formatVersion != SARC_VERSION) {
        std::cerr << "[SARCObject::SARCObject] Expected SARC version 0x" <<
            std::hex << SARC_VERSION << ", got version 0x" <<
            header->formatVersion  << " instead!";
        return;
    }

    const SfatSection* sfatSection = reinterpret_cast<const SfatSection*>(data + header->sectionSize);
    if (sfatSection->magic != SFAT_MAGIC) {
        std::cerr << "[SARCObject::SARCObject] Invalid SARC binary: SFAT section magic is nonmatching!\n";
        return;
    }

    const SfntSection* sfntSection = reinterpret_cast<const SfntSection*>(
        sfatSection->nodes + sfatSection->nodeCount
    );
    if (sfntSection->magic != SFNT_MAGIC) {
        std::cerr << "[SARCObject::SARCObject] Invalid SARC binary: SFNT section magic is nonmatching!\n";
        return;
    }

    for (unsigned i = 0; i < sfatSection->nodeCount; i++) {
        const SfatNode* node = sfatSection->nodes + i;

        const char* name = sfntSection->data + (node->nameOffset * 4);

        const unsigned char* nodeDataStart = data + header->dataStart + node->dataOffsetStart;
        const unsigned char* nodeDataEnd = data + header->dataStart + node->dataOffsetEnd;

        // Create file at path.
        {
            std::stringstream stream(name);
            std::string currentSegment;
            
            Directory* currentDir = &this->structure;

            // Traverse.
            while (std::getline(stream, currentSegment, '/')) {
                // Last segment is the file name; add it!
                if (stream.peek() == EOF) {
                    File newFile(currentSegment);
                    newFile.parent = currentDir;
                    newFile.data = std::vector<unsigned char>(nodeDataStart, nodeDataEnd);

                    currentDir->AddFile(std::move(newFile));

                    break;
                }

                // Find or create the subdirectory.
                auto it = std::find_if(
                    currentDir->subdirectories.begin(), currentDir->subdirectories.end(),
                    [&currentSegment](const Directory& dir) {
                        return dir.name == currentSegment;
                    }
                );

                // Directory doesn't exist; create and add it.
                if (it == currentDir->subdirectories.end()) {
                    Directory newDir(currentSegment, currentDir);
                    currentDir->AddDirectory(std::move(newDir));

                    currentDir = &currentDir->subdirectories.back();
                }
                // Move into the existing directory.
                else
                    currentDir = &(*it);
            }
        }
    }
}

std::optional<SARCObject> readNZlibSARC(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[SARC::readNZlibSARC] Error opening file at path: " << filePath << '\n';
        return std::nullopt;
    }

    const std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    unsigned char* buffer = new unsigned char[fileSize];
    file.read(reinterpret_cast<char*>(buffer), fileSize);

    file.close();

    // yaz0 decompress
    const auto decompressedData = NZlib::decompress(
        buffer,
        fileSize
    );

    delete[] buffer;

    if (!decompressedData.has_value()) {
        std::cerr << "[SARC::readNZlibSARC] Error decompressing file at path: " << filePath << '\n';
        return std::nullopt;
    }

    SARCObject archive(
        decompressedData->data(),
        decompressedData->size()
    );

    return archive;
}

File* findFile(const char* path, Directory& directory) {
    const char* slash = strchr(path, '/');

    if (slash == nullptr) { // Slash not found: it's a file, search for it
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

}
