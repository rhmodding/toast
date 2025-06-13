#include "FileUtil.hpp"

#include <cstdio>
#include <fstream>

#include "Logging.hpp"

bool FileUtil::doesFileExist(std::string_view filePath) {
    if (filePath.empty())
        return false;

    // Apparently, the fastest way to do this is with fopen
    FILE* file = fopen(filePath.data(), "rb");
    if (!file)
        return false;

    fclose(file);
    return true;
}

bool FileUtil::copyFile(std::string_view filePathSrc, std::string_view filePathDst, bool overwrite) {
    if (doesFileExist(filePathDst))
        return true;

    std::ifstream src(filePathSrc.data(), std::ios::binary);
    if (!src) {
        Logging::err << "[FileUtil::copyFile] Unable to open source file at path \"" << filePathSrc << "\"!" << std::endl;
        return false;
    }

    std::ofstream dst(filePathDst.data(), std::ios::binary);
    if (!dst) {
        Logging::err << "[FileUtil::copyFile] Unable to open destination file at path \"" << filePathDst << "\"!" << std::endl;
        return false;
    }

    dst << src.rdbuf();

    return true;
}
