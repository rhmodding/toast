#include "CxxDemangle.hpp"

#include <cstdlib>

#include <cxxabi.h>

std::string CxxDemangle::Demangle(const char* mangledName) {
    int demangleStatus = 0;
    char* demangledName = abi::__cxa_demangle(mangledName, nullptr, nullptr, &demangleStatus);

    if (demangledName) {
        std::string result (demangledName);
        std::free(demangledName);

        return result;
    }
    else
        return std::string();
}
