#include "Archive.hpp"

#include <cstring>

#include <cstddef>

void Archive::Directory::SortAlphabetic() {
    this->files.sort([](const File& a, const File& b) {
        return a.name < b.name;
    });
    this->subdirectories.sort([](const Directory& a, const Directory& b) {
        return a.name < b.name;
    });
}

const Archive::File* Archive::findFile(std::string_view path, const Archive::Directory& directory) {
    const size_t slashOffset = path.find('/');

    // Slash not found: it's a file, search for it
    if (slashOffset == std::string_view::npos) {
        for (const File& file : directory.files) {
            if (file.name == path)
                return &file;
        }

        return nullptr;
    }
    // Slash found: it's a subdirectory, recursive search
    else {
        for (const Directory& subDir : directory.subdirectories) {
            if (strncmp(subDir.name.data(), path.data(), slashOffset) == 0)
                return findFile(path.substr(slashOffset + 1), subDir);
        }

        return nullptr;
    }
}
