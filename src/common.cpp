#include "common.hpp"

#include <fstream>

#include <cmath>

#include <bit>

#include "MainThreadTaskManager.hpp"

namespace Common {

float byteswapFloat(float value) {
    return std::bit_cast<float, uint32_t>(
        BYTESWAP_32(std::bit_cast<uint32_t, float>(value))
    );
}

bool checkIfFileExists(const char* filePath) {
    if (!filePath)
        return false;

    if (FILE* file = fopen(filePath, "r")) {
        fclose(file);
        return true;
    }
    return false;
}

// TODO: remove
bool SaveBackupFile(const char* filePath, bool once) {
    char newFileName[PATH_MAX];
    snprintf(newFileName, PATH_MAX, "%s.bak", filePath);

    if (FILE* file = fopen(newFileName, "r")) {
        fclose(file);
        if (once)
            return true;
    }

    std::ifstream src(filePath, std::ios::binary);
    if (!src) {
        std::cerr << "[Common::SaveBackupFile] Error: Unable to open source file at path: " << filePath << '\n';
        return false;
    }

    std::ofstream dst(newFileName, std::ios::binary);
    if (!dst) {
        std::cerr << "[Common::SaveBackupFile] Error: Unable to create backup file at path: " << newFileName << '\n';
        return false;
    }

    dst << src.rdbuf();

    std::cout << "[Common::SaveBackupFile] Successfully created backup at path: " << newFileName << '\n';
    return true;
}

} // namespace Common
