#include "Files.hpp"

#include <cstdio>
#include <fstream>

#include "Logging.hpp"

// Apparently, the fastest way to do this is with fopen
bool Files::doesFileExist(std::string_view filePath) {
    if (filePath.empty())
        return false;

    FILE* file = fopen(filePath.data(), "rb");
    if (!file)
        return false;

    fclose(file);
    return true;
}

bool Files::copyFile(std::string_view filePathSrc, std::string_view filePathDst, bool overwrite) {
    if (doesFileExist(filePathDst))
        return true;

    std::ifstream src(filePathSrc, std::ios::binary);
    if (!src) {
        Logging::err << "[Files::copyFile] Unable to open source file at path \"" << filePathSrc << "\"!" << std::endl;
        return false;
    }

    std::ofstream dst(filePathDst, std::ios::binary);
    if (!dst) {
        Logging::err << "[Files::copyFile] Unable to open destination file at path \"" << filePathDst << "\"!" << std::endl;
        return false;
    }

    dst << src.rdbuf();

    return true;
}
