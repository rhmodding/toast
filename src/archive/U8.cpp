#include "U8.hpp"

#include "../common.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <optional>
#include <cstdint>

#include "../compression/Yaz0.hpp"
#include <stack>

#pragma pack(push, 1) // Pack struct members tightly without padding

#define HEADER_MAGIC 0x55AA382D

struct U8ArchiveHeader {
    // Magic value (should always equal to 0x55AA382D [1437218861] if valid)
    uint32_t magic;

    // Offset to the root (first) node, always 0x20
    int32_t rootNodeOffset;

    // Size of header from the root node to the end of string pool
    int32_t headerSize;

    // Offset to data: this is rootNodeOffset + headerSize, aligned to 0x40
    int32_t dataOffset;

    // Reserved bytes. These are always present in the header
    uint32_t reserved[4];
};

struct U8ArchiveNode {
    // 0x0: file, 0x1: directory
    uint8_t type;

    // Offset into the string pool for the name of the file/dir.
    // This is technically a uint24_t but since that type isn't avaliable we use 3 uint8_t's in an array
    uint8_t nameOffset[3];

    // File: Offset of begin of data
    // Directory: Index of the parent directory
    uint32_t dataOffset;

    // File: Size of data
    // Directory: Amount of all child nodes (including subnodes)
    uint32_t size;
};

#pragma pack(pop) // Reset packing alignment to default

namespace U8 {
    File::File(const std::string& n) : name(n) {}

    Directory::Directory(const std::string& n) : name(n) {}
    Directory::Directory(const std::string& n, Directory* parentDir) : name(n), parent(parentDir) {}

    void Directory::AddFile(File& file) {
        file.parent = this;
        files.push_back(file);
    }

    void Directory::AddDirectory(Directory& directory) {
        directory.parent = this;
        subdirectories.push_back(directory);
    }

    Directory* Directory::GetParent() const {
        return parent;
    }

    U8ArchiveObject::U8ArchiveObject(const char* archiveData, const size_t dataSize) {
        const U8ArchiveHeader* header = reinterpret_cast<const U8ArchiveHeader*>(archiveData);

        if (BYTESWAP_32(header->magic) != 0x55AA382D) {
            std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: header magic failed check!\n";
            return;
        }

        std::vector<U8ArchiveNode> nodes;

        size_t readOffset = BYTESWAP_32(header->rootNodeOffset);

        // Read root node
        U8ArchiveNode rootNode;
        Common::ReadAtOffset(archiveData, readOffset, dataSize, rootNode);
        
        rootNode.size = BYTESWAP_32(rootNode.size);
        rootNode.dataOffset = BYTESWAP_32(rootNode.dataOffset);

        nodes.push_back(rootNode);

        this->structure.totalNodeCount = rootNode.size;

        // Read rest of nodes
        for (uint32_t i = 0; i < this->structure.totalNodeCount - 1; i++) {
            U8ArchiveNode node;
            Common::ReadAtOffset(archiveData, readOffset, dataSize, node);

            node.size = BYTESWAP_32(node.size);
            node.dataOffset = BYTESWAP_32(node.dataOffset);

            nodes.push_back(node);
        }

        // Read string pool
        std::vector<std::string> stringList;
        for (uint32_t i = 0; i < this->structure.totalNodeCount; i++) {
            std::string str;

            char c;
            while (true) {
                Common::ReadAtOffset(archiveData, readOffset, dataSize, c);

                if (c == 0x0)
                    break;

                str.push_back(c);
            }

            stringList.push_back(str);
        }

        // std::unordered_map<size_t, U8ArchiveNode*> parentMap;

        uint16_t dirStack[16];
        uint16_t dirIndex{ 0 };
        Directory* currentDirectory{ &this->structure };

        // Read nodes, except for first (root directory)
        for (size_t i = 1; i < nodes.size(); i++) {
            U8ArchiveNode node = nodes[i];

            if (node.type == 0x0) {
                File file(stringList[i]);

                file.data.resize(node.size);

                readOffset = node.dataOffset;
                for (uint32_t j = 0; j < node.size; j++) {
                    char c;
                    Common::ReadAtOffset(archiveData, readOffset, dataSize, c);

                    file.data[j] = c;
                }

                currentDirectory->AddFile(file);
            }
            else if (node.type == 0x1) {
                Directory directory(stringList[i]);

                directory.totalNodeCount = node.size;

                currentDirectory->AddDirectory(directory);

                currentDirectory = &currentDirectory->subdirectories.back();
                dirStack[++dirIndex] = node.size;
            }
            else { // Bad type, log error and return
                std::cerr << "[U8ArchiveObject::U8ArchiveObject] Invalid U8 binary: invalid node type (" << std::to_string(node.type) << ")!\n";
                return;
            }

            while (dirStack[dirIndex] == i + 1 && dirIndex > 0) {
                currentDirectory = currentDirectory->GetParent();
                dirIndex--;
            }
        }

        return;
    }

    std::vector<char> U8ArchiveObject::Reserialize() {
        // TODO: implement

        return std::vector<char>();
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
        auto decompressedData = Yaz0::decompress(fileContent.data(), fileSize);
        if (!decompressedData.has_value()) {
            std::cerr << "[U8::readYaz0U8Archive] Error decompressing file at path: " << filePath << '\n';
            return std::nullopt;
        }

        U8::U8ArchiveObject archive(decompressedData.value().data(), decompressedData.value().size());

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