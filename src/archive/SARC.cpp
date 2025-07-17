#include "SARC.hpp"

#include <cstdint>

#include "Logging.hpp"

#include <fstream>

#include <sstream>

#include <algorithm>

#include <stack>

#include "compression/NZlib.hpp"

#include "Macro.hpp"

constexpr uint32_t SARC_MAGIC = IDENTIFIER_TO_U32('S','A','R','C');
constexpr uint32_t SFAT_MAGIC = IDENTIFIER_TO_U32('S','F','A','T');
constexpr uint32_t SFNT_MAGIC = IDENTIFIER_TO_U32('S','F','N','T');

constexpr uint16_t SARC_VERSION = 0x0100;

constexpr uint16_t SARC_BYTEORDER_BIG    = 0xFFFE;
constexpr uint16_t SARC_BYTEORDER_LITTLE = 0xFEFF;

constexpr uint32_t SARC_DEFAULT_HASH_KEY = 0x65;

struct SarcFileHeader {
    uint32_t magic { SARC_MAGIC }; // Compare to SARC_MAGIC.
    uint16_t headerSize { sizeof(SarcFileHeader) };

    uint16_t byteOrder { SARC_BYTEORDER_LITTLE }; // Compare to SARC_BYTEORDER_BIG and SARC_BYTEORDER_LITTLE.

    uint32_t fileSize; // Size of this file.
    uint32_t dataStart; // File offset to the archive data.

    uint16_t formatVersion { SARC_VERSION }; // Compare to SARC_VERSION.

    uint16_t _reserved { 0x0000 };
} __attribute__((packed));

struct SfatNode {
    uint32_t nameHash; // See sarcComputeHash for algorithm.

    // | nameOffset (3byte) | collisionCount (1byte) |
    uint32_t nameOffsetAndCollisionCount;

    // Relative to the start of the SFNT section's data.
    unsigned getNameOffset() const {
        return (this->nameOffsetAndCollisionCount & 0x00FFFFFF) * 4;
    }

    // The collision count represents the occourence number of a file entry's hash.
    // The value is usually 1, assuming no collisions.
    unsigned getCollisionCount() const {
        return (this->nameOffsetAndCollisionCount & 0xFF000000) >> 24;
    }

    void setCollisionCount(unsigned collisionCount) {
        this->nameOffsetAndCollisionCount &= 0x00FFFFFF;
        this->nameOffsetAndCollisionCount |= ((collisionCount & 0xFF) << 24);
    }

    // Note: nameOffset must be divisible by four
    void setNameOffset(unsigned nameOffset) {
        this->nameOffsetAndCollisionCount &= 0xFF000000;
        this->nameOffsetAndCollisionCount |= ((nameOffset / 4) & 0xFFFFFF);
    }

    uint32_t dataOffsetStart; // Relative to the file header's dataStart.
    uint32_t dataOffsetEnd; // Relative to the file header's dataStart.
} __attribute__((packed));

struct SfatSection {
    uint32_t magic { SFAT_MAGIC }; // Compare to SFAT_MAGIC.
    uint16_t sectionSize { sizeof(SfatSection) };

    uint16_t nodeCount;

    // SARC_DEFAULT_HASH_KEY by default; may differ in the case of
    // a collision.
    uint32_t hashKey;

    SfatNode nodes[0];
} __attribute__((packed));

struct SfntSection {
    uint32_t magic { SFNT_MAGIC }; // Compare to SFNT_MAGIC.
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

namespace Archive {

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
        const char* name = sfntSection->data + node->getNameOffset();

        const unsigned char* nodeDataStart = data + header->dataStart + node->dataOffsetStart;
        const unsigned char* nodeDataEnd = data + header->dataStart + node->dataOffsetEnd;

        // Create file at path.

        std::stringstream stream(name);
        std::string currentSegment;

        Archive::Directory* currentDir = &mStructure;

        // Traverse.
        while (std::getline(stream, currentSegment, '/')) {
            // Last segment is the file name; add it!
            if (stream.peek() == EOF) {
                Archive::File newFile(currentSegment);

                newFile.parent = currentDir;
                newFile.data = std::vector<unsigned char>(nodeDataStart, nodeDataEnd);

                currentDir->AddFile(std::move(newFile));

                break;
            }

            // Find or create the subdirectory.
            auto it = std::find_if(
                currentDir->subdirectories.begin(), currentDir->subdirectories.end(),
                [&currentSegment](const Archive::Directory& dir) {
                    return dir.name == currentSegment;
                }
            );

            // Directory doesn't exist; create and add it.
            if (it == currentDir->subdirectories.end()) {
                Archive::Directory newDir(currentSegment, currentDir);
                currentDir->AddDirectory(std::move(newDir));

                currentDir = &currentDir->subdirectories.back();
            }
            // Move into the existing directory.
            else
                currentDir = &(*it);
        }
    }

    mInitialized = true;
}

std::vector<unsigned char> SARCObject::Serialize() {
    struct FileEntry {
        const Archive::File* file;
        std::string path;
        uint32_t collisionCount;
        uint32_t pathHash;
    };
    std::vector<FileEntry> entries;

    std::stack<const Archive::Directory*> dirStack;
    dirStack.push(&mStructure);

    while (!dirStack.empty()) {
        const Archive::Directory* currentDir = dirStack.top();
        dirStack.pop();

        for (const auto& file : currentDir->files) {
            // Build file path.
            std::string path = file.name;

            const Archive::Directory* dir = file.parent;
            while (dir && dir->parent) {
                path = dir->name + "/" + path;
                dir = dir->parent;
            }

            entries.push_back({
                .file = &file,
                .path = path,
                .pathHash = sarcComputeHash(path, SARC_DEFAULT_HASH_KEY)
            });
        }
        for (const auto& subdir : currentDir->subdirectories)
            dirStack.push(&subdir);
    }

    // SARC nodes are always sorted by their hashes.
    std::sort(
        entries.begin(), entries.end(),
        [](const FileEntry& a, const FileEntry& b) {
            return a.pathHash < b.pathHash;
        }
    );

    uint32_t lastHash = entries[0].pathHash;
    uint32_t lastCollisionCount = 0;
    for (auto& entry : entries) {
        if (entry.pathHash == lastHash) {
            entry.collisionCount = ++lastCollisionCount;
        }
        else {
            lastHash = entry.pathHash;
            lastCollisionCount = 1;
            entry.collisionCount = lastCollisionCount;
        }
    }

    size_t fullSize = sizeof(SarcFileHeader) + sizeof(SfatSection) +
        sizeof(SfntSection) + (sizeof(SfatNode) * entries.size());
    for (size_t i = 0; i < entries.size(); i++) {
        fullSize += ALIGN_UP_4(entries[i].path.size() + 1);
        fullSize = ALIGN_UP_32(fullSize) + ALIGN_UP_32(entries[i].file->data.size());
    }
    std::vector<unsigned char> result(fullSize);

    SarcFileHeader* header = reinterpret_cast<SarcFileHeader*>(result.data());
    *header = SarcFileHeader {
        .fileSize = static_cast<uint32_t>(fullSize)
    };

    SfatSection* sfatSection = reinterpret_cast<SfatSection*>(result.data() + header->headerSize);
    *sfatSection = SfatSection {
        .nodeCount = static_cast<uint16_t>(entries.size()),

        .hashKey = SARC_DEFAULT_HASH_KEY
    };

    SfntSection* sfntSection = reinterpret_cast<SfntSection*>(
        sfatSection->nodes + sfatSection->nodeCount
    );
    *sfntSection = SfntSection {};

    // Write the strings & nodes at the same time.

    char* currentName = sfntSection->data;
    for (size_t i = 0; i < entries.size(); i++) {
        const FileEntry& entry = entries[i];

        SfatNode* node = sfatSection->nodes + i;

        node->nameHash = entry.pathHash;

        node->setCollisionCount(entry.collisionCount);
        node->setNameOffset(currentName - sfntSection->data);

        strcpy(currentName, entry.path.c_str());
        currentName += ALIGN_UP_4(entry.path.size() + 1);
    }

    // Write the data.

    unsigned char* dataStart = reinterpret_cast<unsigned char*>(currentName);

    header->dataStart = static_cast<uint32_t>(dataStart - result.data());

    unsigned char* currentData = dataStart;
    for (size_t i = 0; i < entries.size(); i++) {
        const FileEntry& entry = entries[i];

        SfatNode* node = sfatSection->nodes + i;

        node->dataOffsetStart = static_cast<uint32_t>(currentData - dataStart);
        node->dataOffsetEnd = node->dataOffsetStart + entry.file->data.size();

        memcpy(currentData, entry.file->data.data(), entry.file->data.size());
        currentData += ALIGN_UP_32(entry.file->data.size());
    }

    return result;
}

} // namespace Archive
