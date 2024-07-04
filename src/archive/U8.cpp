#include "U8.hpp"

#include <cstdint>

#include <iostream>
#include <fstream>

#include <bitset>

#include <algorithm>

#include <unordered_map>
#include <stack>

#include "../common.hpp"

#include "../compression/Yaz0.hpp"

#pragma pack(push, 1) // Pack struct members tightly without padding

#define HEADER_MAGIC 0x2D38AA55

struct U8ArchiveHeader {
    // Magic value (should always equal to 0x55AA382D [1437218861] if valid)
    uint32_t magic;

    // Offset to the root (first) node, always 0x20
    uint32_t rootNodeOffset;

    // Offset to data starting from the root node
    uint32_t headerSize;

    // Offset to data: this is rootNodeOffset + headerSize, aligned to 0x40
    uint32_t dataOffset;

    // Reserved bytes. These are always present in the header
    uint32_t reserved[4];
};

struct U8ArchiveNode {
    uint32_t attributes; // uint8: type, uint24: nameOffset

    // 0x00: file, 0x01: directory
    inline uint8_t getType() const {
        return static_cast<uint8_t>(this->attributes & 0xFF);
    }

    // Offset into the string pool for the name of the node.
    inline uint32_t getNameOffset() const { 
       return BYTESWAP_32((this->attributes >> 8) & 0xFFFFFF);
    }

    inline void setType(uint8_t type) {
        this->attributes &= 0xFFFFFF00;
        this->attributes |= static_cast<uint32_t>(type);
    }

    inline void setNameOffset(uint32_t offset) {
        this->attributes &= 0x000000FF;
        this->attributes |= (BYTESWAP_32(offset) & 0xFFFFFF) << 8;
    }

    // File: Offset of begin of data
    // Directory: Index of the parent directory
    uint32_t dataOffset;

    // File: Size of data
    // Directory: Index of the first node that is not part of this directory
    uint32_t size;
};

#pragma pack(pop) // Reset packing alignment to default

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

        if (header->magic != HEADER_MAGIC) {
            std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: header magic failed check!\n";
            return;
        }

        const U8ArchiveNode* rootNode = reinterpret_cast<const U8ArchiveNode*>(
            archiveData + BYTESWAP_32(header->rootNodeOffset)
        );

        uint32_t rootSize = BYTESWAP_32(reinterpret_cast<const U8ArchiveNode*>(
            archiveData + BYTESWAP_32(header->rootNodeOffset)
        )->size);

        std::vector<std::string> stringList(rootSize);
        {
            const char* stringPool = reinterpret_cast<const char*>(
                archiveData + sizeof(U8ArchiveHeader) + (rootSize * sizeof(U8ArchiveNode))
            );

            for (uint32_t i = 0, offset = 0; i < rootSize; i++) {
                std::string str(stringPool + offset);
                offset += str.size() + 1; // Null terminator

                stringList.at(i) = std::move(str);
            }
        }

        uint16_t dirStack[16]; // 16 layers of depth
        uint16_t dirIndex{ 0 };
        Directory* currentDirectory{ &this->structure };

        // Read nodes, except for first (root node)
        for (uint32_t i = 1; i < rootSize; i++) {
            const U8ArchiveNode* node = reinterpret_cast<const U8ArchiveNode*>(
                archiveData + sizeof(U8ArchiveHeader) + 
                (i * sizeof(U8ArchiveNode))
            );

            if (node->getType() == 0x00) {
                File file(stringList.at(i));

                const unsigned char* dataStart = archiveData + BYTESWAP_32(node->dataOffset);
                file.data = std::vector<unsigned char>(
                    dataStart,
                    dataStart + BYTESWAP_32(node->size)
                );

                currentDirectory->AddFile(file);
            }
            else if (node->getType() == 0x01) {
                Directory directory(stringList.at(i));

                currentDirectory->AddDirectory(directory);

                currentDirectory = &currentDirectory->subdirectories.back();
                dirStack[++dirIndex] = BYTESWAP_32(node->size);
            }
            else {
                std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: invalid node type (" << std::to_string(node->getType()) << ")!\n";
                return;
            }

            while (dirStack[dirIndex] == i + 1 && dirIndex > 0) {
                currentDirectory = currentDirectory->GetParent();
                dirIndex--;
            }
        }

        return;
    }

    std::vector<unsigned char> U8ArchiveObject::Reserialize() {
        std::vector<unsigned char> result(sizeof(U8ArchiveHeader));

        U8ArchiveHeader* header = reinterpret_cast<U8ArchiveHeader*>(result.data());
        header->magic = HEADER_MAGIC;
        
        header->rootNodeOffset = BYTESWAP_32(sizeof(U8ArchiveHeader));

        // Set headerSize and dataOffset later.

        header->reserved[0] = 0x0000;
        header->reserved[1] = 0x0000;
        header->reserved[2] = 0x0000;
        header->reserved[3] = 0x0000;

        //////////////////////////////////////////////////////////////////

        struct FlatEntry {
            // 0x02 for Root Node, 0x01 for Directory* and 0x00 for File*
            uint8_t type;

            void* ptr;

            uint32_t parent;

            uint32_t nextOutOfDir;
        };

        std::vector<FlatEntry> flattenedArchive;
        flattenedArchive.push_back({ 0x02, nullptr, 0, 0 });

        std::stack<std::pair<Directory*, uint32_t>> directoryStack;
        directoryStack.push({ &this->structure, 0 });

        std::vector<uint32_t> parentList;
        parentList.push_back(0);

        while (!directoryStack.empty()) {
            Directory* currentDir = directoryStack.top().first;
            uint32_t& index = directoryStack.top().second;

            if (index < currentDir->files.size()) {
                flattenedArchive.push_back({ 0x00, &currentDir->files[index], parentList.back(), 0 });

                ++index;
            } else if (index < currentDir->files.size() + currentDir->subdirectories.size()) {
                index -= static_cast<uint32_t>(currentDir->files.size());
                Directory* subDir = &currentDir->subdirectories[index];

                flattenedArchive.push_back({ 0x01, subDir, parentList.back(), 0 });

                parentList.push_back(static_cast<uint32_t>(flattenedArchive.size() - 1));
                directoryStack.push({ subDir, 0});
                ++index;
            } else {
                for (uint32_t parentIndex : parentList)
                    flattenedArchive.at(parentIndex).nextOutOfDir = static_cast<uint32_t>(flattenedArchive.size());

                parentList.pop_back();
                directoryStack.pop();
            }
        }

        uint32_t stringPoolSize{ 1 };
        uint32_t dataSize{ 0 };

        std::vector<uint32_t> stringOffsets;
        std::vector<uint32_t> dataOffsets;

        // String offsets
        for (const FlatEntry& entry : flattenedArchive) {
            if (entry.type == 0x01) {
                stringOffsets.push_back(stringPoolSize);
                stringPoolSize += static_cast<uint32_t>(reinterpret_cast<Directory*>(entry.ptr)->name.size() + 1);
            }
            else if (entry.type == 0x00) {
                stringOffsets.push_back(stringPoolSize);
                stringPoolSize += static_cast<uint32_t>(reinterpret_cast<File*>(entry.ptr)->name.size() + 1);
            }
            else
                stringOffsets.push_back(0);
        }

        uint32_t dataOffset = static_cast<uint32_t>(
            sizeof(U8ArchiveHeader) +
            (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
            stringPoolSize
        );

        dataOffset += 0x20 - (dataOffset % 0x20);

        // Data offsets
        for (uint32_t i = 0; i < flattenedArchive.size(); i++) {
            const FlatEntry& entry = flattenedArchive.at(i);

            if (entry.type == 0x00) {
                dataOffsets.push_back(dataOffset + dataSize);
                dataSize += static_cast<uint32_t>(reinterpret_cast<File*>(entry.ptr)->data.size());

                if (i+1 != flattenedArchive.size())
                    dataSize = (dataSize + 31) & ~31;
            }
            else
                dataOffsets.push_back(0);
        }

        header->dataOffset = BYTESWAP_32(dataOffset);
        header->headerSize = BYTESWAP_32(static_cast<uint32_t>(
            (sizeof(U8ArchiveNode) * flattenedArchive.size()) + stringPoolSize
        ));

        result.resize(dataOffset + dataSize);

        // Write all data
        for (uint32_t i = 0; i < flattenedArchive.size(); i++) {
            FlatEntry& entry = flattenedArchive.at(i);

            U8ArchiveNode* node = reinterpret_cast<U8ArchiveNode*>(
                result.data() + sizeof(U8ArchiveHeader) +
                (sizeof(U8ArchiveNode) * i)
            );

            switch (entry.type) {
                case 0x02: { // Root node
                    node->setType(0x01);

                    node->size = BYTESWAP_32(entry.nextOutOfDir);

                    node->dataOffset = 0x0000;
                    node->setNameOffset(0x000000);
                } break;

                case 0x01: { // Directory
                    node->setType(0x01);

                    node->size = BYTESWAP_32(entry.nextOutOfDir);

                    node->dataOffset = BYTESWAP_32(entry.parent);

                    node->setNameOffset(stringOffsets.at(i) & 0xFFFFFF);

                    // Copy string
                    strcpy(
                        reinterpret_cast<char*>(result.data() +
                            sizeof(U8ArchiveHeader) +
                            (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
                            stringOffsets.at(i)
                        ),
                        reinterpret_cast<Directory*>(entry.ptr)->name.c_str()
                    );
                } break;

                case 0x00: { // File
                    std::vector<unsigned char>* data = &reinterpret_cast<File*>(entry.ptr)->data;

                    node->setType(0x00);

                    node->size = BYTESWAP_32(static_cast<uint32_t>(data->size()));

                    node->dataOffset = BYTESWAP_32(dataOffsets.at(i));

                    node->setNameOffset(stringOffsets.at(i) & 0xFFFFFF);

                    // Copy string
                    strcpy(
                        reinterpret_cast<char*>(result.data() +
                            sizeof(U8ArchiveHeader) +
                            (sizeof(U8ArchiveNode) * flattenedArchive.size()) +
                            stringOffsets.at(i)
                        ),
                        reinterpret_cast<File*>(entry.ptr)->name.c_str()
                    );

                    // Copy data
                    memcpy(result.data() + dataOffsets.at(i), data->data(), data->size());
                } break;

                default:
                    break;
            }
        }

        return result;
    }

    std::optional<U8ArchiveObject> readYaz0U8Archive(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[U8::readYaz0U8Archive] Error opening file at path: " << filePath << '\n';
            return std::nullopt;
        }

        // file read
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> fileContent(fileSize);
        file.read(fileContent.data(), fileSize);

        file.close();

        // yaz0 decompress
        auto decompressedData = Yaz0::decompress(
            reinterpret_cast<unsigned char*>(fileContent.data()),
            fileSize
        );

        if (!decompressedData.has_value()) {
            std::cerr << "[U8::readYaz0U8Archive] Error decompressing file at path: " << filePath << '\n';
            return std::nullopt;
        }

        U8::U8ArchiveObject archive(
            (*decompressedData).data(),
            (*decompressedData).size()
        );

        return archive;
    }

    std::optional<File> findFile(const std::string& path, const Directory& directory) {
        size_t pos = path.find('/');

        if (pos == std::string::npos) {
            // No '/' found: It's a file, go find it
            for (const File& file : directory.files)
                if (file.name == path)
                    return file;

            // File not found
            return std::nullopt;
        }
        else {
            // '/' found: It's a subdirectory, recursively search
            std::string subDirName = path.substr(0, pos);
            std::string remainingPath = path.substr(pos + 1);

            for (const Directory& subDir : directory.subdirectories)
                if (subDir.name == subDirName)
                    return findFile(remainingPath, subDir);

            // Subdirectory not found
            return std::nullopt;
        }
    }
}