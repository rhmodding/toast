#include "MyPathUtil.hpp"

#include <cstdint>

#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

namespace MyPathUtil {

std::string get() {
#if defined(_WIN32)

    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    return std::string(buffer);

#elif defined(__APPLE__)

    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return std::string(buffer);
    }
    else { // Try harder ..
        std::string pathString;
        pathString.resize(size);

        if (_NSGetExecutablePath(pathString.data(), &size) == 0) {
            pathString.resize(std::strlen(pathString.data()));
            return pathString;
        }
        else {
            // Uh ???????
            return "";
        }
    }

#else // probably Linux ¯\_(ツ)_/¯

    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (count != -1) {
        return std::string(buffer, count);
    }
    else {
        // Uh ???????
        return "";
    }

#endif
}

} // namespace MyPathUtil
