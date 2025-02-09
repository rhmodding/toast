#include "files.hpp"

#include <cstdio>
#include <fstream>

#include <iostream>

bool Files::doesFileExist(const char* filePath) {
    if (filePath == nullptr)
        return false;

    FILE* file = fopen(filePath, "r");
    if (file == nullptr)
        return false;

    fclose(file);
    return true;
}

bool Files::BackupFile(const char* filePath, bool once) {
    char newFileName[PATH_MAX];
    snprintf(newFileName, PATH_MAX, "%s.bak", filePath);

    if (FILE* file = fopen(newFileName, "r")) {
        fclose(file);
        if (once)
            return true;
    }

    std::ifstream src(filePath, std::ios::binary);
    if (!src) {
        std::cerr << "[Files::BackupFile] Error: Unable to open source file at path: " << filePath << '\n';
        return false;
    }

    std::ofstream dst(newFileName, std::ios::binary);
    if (!dst) {
        std::cerr << "[Files::BackupFile] Error: Unable to create backup file at path: " << newFileName << '\n';
        return false;
    }

    dst << src.rdbuf();

    return true;
}
