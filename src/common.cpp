#include "common.hpp"

#include <fstream>

#include <cmath>

#include <bit>

#include "MtCommandManager.hpp"

namespace Common {

float byteswapFloat(float value) {
    return std::bit_cast<float, uint32_t>(
        BYTESWAP_32(std::bit_cast<uint32_t, float>(value))
    );
}

// TODO: remove
bool SaveBackupFile(const char* filePath, bool once) {
    char newFileName[PATH_MAX];
    snprintf(newFileName, PATH_MAX, "%s.bak", filePath);

    bool exists { false };
    {
        if (FILE *file = fopen(newFileName, "r")) {
            fclose(file);
            exists = true;
        }
    }

    if (once && exists)
        return true;

    std::ifstream src(filePath,    std::ios::binary);
    std::ofstream dst(newFileName, std::ios::binary);

    if (!src.is_open()) {
        std::cerr << "[Common::SaveBackupFile] Error opening source file at path: " << filePath << '\n';
        return false;
    }
    if (!dst.is_open()) {
        std::cerr << "[Common::SaveBackupFile] Error opening destination file at path: " << newFileName << '\n';
        return false;
    }

    dst << src.rdbuf();

    src.close();
    dst.close();

    std::cout << "[Common::SaveBackupFile] Successfully cloned file at path: " << filePath << '\n';

    return true;
}

void FitRect(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale) {
    float widthRatio = targetRect.x / rectToFit.x;
    float heightRatio = targetRect.y / rectToFit.y;

    scale = std::min(widthRatio, heightRatio);

    rectToFit.x *= scale;
    rectToFit.y *= scale;
}

float EaseInOut(float t) {
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
}
float EaseIn(float t) {
    return t * t;
}
float EaseOut(float t) {
    return 1 - (1 - t) * (1 - t);
}

} // namespace Common
