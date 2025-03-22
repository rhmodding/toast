#include "SARC.hpp"

#include <cstdint>

#include "../Logging.hpp"

#include <fstream>

#include <sstream>

#include <algorithm>

#include <stack>

#include "../compression/NZlib.hpp"

#include "../common.hpp"

constexpr uint32_t SARC_MAGIC = IDENTIFIER_TO_U32('S','A','R','C');
constexpr uint32_t SFAT_MAGIC = IDENTIFIER_TO_U32('S','F','A','T');
constexpr uint32_t SFNT_MAGIC = IDENTIFIER_TO_U32('S','F','N','T');

constexpr uint16_t SARC_VERSION = 0x0100;

constexpr uint16_t SARC_BYTEORDER_BIG    = 0xFFFE;
constexpr uint16_t SARC_BYTEORDER_LITTLE = 0xFEFF;

constexpr uint32_t SARC_DEFAULT_HASH_KEY = 0x65;

struct SarcFileHeader {
    uint32_t magic { SARC_MAGIC }; // Compare to SARC_MAGIC
    uint16_t headerSize { sizeof(SarcFileHeader) };

    uint16_t byteOrder { SARC_BYTEORDER_LITTLE }; // Compare to SARC_BYTEORDER_BIG and SARC_BYTEORDER_LITTLE

    uint32_t fileSize; // Size of this file.
    uint32_t dataStart; // File offset to the archive data.

    uint16_t formatVersion { SARC_VERSION }; // Compare to SARC_VERSION

    uint16_t _reserved { 0x0000 };
} __attribute__((packed));

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
} __attribute__((packed));

struct SfatSection {
    uint32_t magic { SFAT_MAGIC }; // Compare to SFAT_MAGIC
    uint16_t sectionSize { sizeof(SfatSection) };

    uint16_t nodeCount;

    // SARC_DEFAULT_HASH_KEY by default; can differ in the case of
    // a collision.
    uint32_t hashKey;

    SfatNode nodes[0];
} __attribute__((packed));

struct SfntSection {
    uint32_t magic { SFNT_MAGIC }; // Compare to SFNT_MAGIC
    uint16_t sectionSize { sizeof(SfntSection) };

    uint16_t _pad16 { 0x0000 };

    char data[0];
} __attribute__((packed));

static uint32_t sarcComputeHash(std::string_view string, uint32_t key) {
    uint32_t result = 0;
	for (char character : string)
		result = character + result * key;

	return result;
}

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
        Logging::err << "[SARCObject::SARCObject] Invalid SARC binary: data size smaller than header size!" << std::endl;
        return;
    }
    
    const SarcFileHeader* header = reinterpret_cast<const SarcFileHeader*>(data);
    if (header->magic != SARC_MAGIC) {
        Logging::err << "[SARCObject::SARCObject] Invalid SARC binary: header magic is nonmatching!" << std::endl;
        return;
    }

    if (header->byteOrder == SARC_BYTEORDER_BIG) {
        Logging::err << "[SARCObject::SARCObject] Big-endian SARC is not supported!" << std::endl;
        return;
    }
    if (header->byteOrder != SARC_BYTEORDER_LITTLE) {
        Logging::err << "[SARCObject::SARCObject] Invalid SARC binary: byte order mark is invalid!" << std::endl;
        return;
    }

    if (header->formatVersion != SARC_VERSION) {
        Logging::err << "[SARCObject::SARCObject] Expected SARC version 0x" <<
            std::hex << SARC_VERSION << ", got version 0x" <<
            header->formatVersion  << " instead!" << std::endl;
        return;
    }

    const SfatSection* sfatSection = reinterpret_cast<const SfatSection*>(data + header->headerSize);
    if (sfatSection->magic != SFAT_MAGIC) {
        Logging::err << "[SARCObject::SARCObject] Invalid SARC binary: SFAT section magic is nonmatching!" << std::endl;
        return;
    }

    const SfntSection* sfntSection = reinterpret_cast<const SfntSection*>(
        sfatSection->nodes + sfatSection->nodeCount
    );
    if (sfntSection->magic != SFNT_MAGIC) {
        Logging::err << "[SARCObject::SARCObject] Invalid SARC binary: SFNT section magic is nonmatching!" << std::endl;
        return;
    }

    for (unsigned i = 0; i < sfatSection->nodeCount; i++) {
        const SfatNode* node = sfatSection->nodes + i;

        // Path to the file, in the format "dir1/dir2/file.ext".
        const char* name = sfntSection->data + (node->nameOffset * 4);

        const unsigned char* nodeDataStart = data + header->dataStart + node->dataOffsetStart;
        const unsigned char* nodeDataEnd = data + header->dataStart + node->dataOffsetEnd;

        // Create file at path.

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

    this->initialized = true;
}

std::vector<unsigned char> SARCObject::Serialize() {
    struct FileEntry {
        const File* file;
        std::string path;
        uint32_t pathHash;
    };
    std::vector<FileEntry> files;

    std::stack<const Directory*> dirStack;
    dirStack.push(&this->structure);

    while (!dirStack.empty()) {
        const Directory* currentDir = dirStack.top();
        dirStack.pop();

        for (const auto& file : currentDir->files) {
            // Build file path.
            std::string path = file.name;

            const Directory* dir = file.parent;
            while (dir && dir->parent) {
                path = dir->name + "/" + path;
                dir = dir->parent;
            }

            files.push_back({
                .file = &file,
                .path = path,
                .pathHash = sarcComputeHash(path, SARC_DEFAULT_HASH_KEY)
            });
        }
        for (const auto& subdir : currentDir->subdirectories)
            dirStack.push(&subdir);
    }
    
    std::sort(files.begin(), files.end(),
    [](const FileEntry& a, const FileEntry& b) {
        return a.pathHash < b.pathHash;
    });

    unsigned fullSize = sizeof(SarcFileHeader) + sizeof(SfatSection) +
        sizeof(SfntSection) + (sizeof(SfatNode) * files.size());
    for (unsigned i = 0; i < files.size(); i++) {
        fullSize += ALIGN_UP_4(files[i].path.size() + 1);
        fullSize = ALIGN_UP_32(fullSize) + ALIGN_UP_32(files[i].file->data.size());
    }
    std::vector<unsigned char> result(fullSize);

    SarcFileHeader* header = reinterpret_cast<SarcFileHeader*>(result.data());
    *header = SarcFileHeader {
        .fileSize = static_cast<uint32_t>(fullSize)
    };

    SfatSection* sfatSection = reinterpret_cast<SfatSection*>(result.data() + header->headerSize);
    *sfatSection = SfatSection {
        .nodeCount = static_cast<uint16_t>(files.size()),

        .hashKey = SARC_DEFAULT_HASH_KEY
    };

    SfntSection* sfntSection = reinterpret_cast<SfntSection*>(
        sfatSection->nodes + sfatSection->nodeCount
    );
    *sfntSection = SfntSection {};

    // Write the strings & nodes at the same time.

    char* currentName = sfntSection->data;
    for (unsigned i = 0; i < files.size(); i++) {
        const FileEntry& file = files[i];

        SfatNode* node = sfatSection->nodes + i;

        node->nameHash = file.pathHash;

        node->collisionCount = 1;
        node->nameOffset = static_cast<uint32_t>(currentName - sfntSection->data) / 4;

        strcpy(currentName, file.path.c_str());
        currentName += ALIGN_UP_4(file.path.size() + 1);
    }

    // Write the data.

    unsigned char* dataStart = reinterpret_cast<unsigned char*>(currentName);
    
    header->dataStart = static_cast<uint32_t>(dataStart - result.data());

    unsigned char* currentData = dataStart;
    for (unsigned i = 0; i < files.size(); i++) {
        const FileEntry& file = files[i];

        SfatNode* node = sfatSection->nodes + i;

        node->dataOffsetStart = static_cast<uint32_t>(currentData - dataStart);
        node->dataOffsetEnd = node->dataOffsetStart + file.file->data.size();

        memcpy(currentData, file.file->data.data(), file.file->data.size());
        currentData += ALIGN_UP_32(file.file->data.size());
    }

    return result;
}

std::optional<SARCObject> readNZlibSARC(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logging::err << "[SARC::readNZlibSARC] Error opening file at path: " << filePath << std::endl;
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
        Logging::err << "[SARC::readNZlibSARC] Error decompressing file at path: " << filePath << std::endl;
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
