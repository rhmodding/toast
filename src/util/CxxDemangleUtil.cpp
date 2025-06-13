#include "CxxDemangleUtil.hpp"

#include <cstdlib>

#include <stdexcept>

#include <cxxabi.h>

std::string demangleStatusToStr(int status) {
    switch (status) {
    case 0:
        return "success";
    case -1:
        return "allocation failure";
    case -2:
        return "invalid mangled name";
    case -3:
        return "invalid argument";
    default:
        return "invalid status";
    }
}

std::string CxxDemangleUtil::Demangle(const char* mangledName) {
    std::string result {};

    int demangleStatus;
    char* demangledName = abi::__cxa_demangle(mangledName, nullptr, nullptr, &demangleStatus);

    if (!demangledName) {
#if !defined(NDEBUG)
        throw std::runtime_error(
            "CxxDemangleUtil::Demangle: failed to demangle \"" + std::string(mangledName) + "\": " +
            demangleStatusToStr(demangleStatus)
        );
#endif
        return result;
    }

    result.assign(demangledName);
    std::free(demangledName);

    return result;
}
