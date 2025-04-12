#include "CxxDemangle.hpp"

#include <cstdlib>

#include <cxxabi.h>

std::string CxxDemangle::Demangle(const char* mangledName) {
    std::string result {};

    int demangleStatus;
    /*  abi::__cxa_demangle status codes

        +0: success
        -1: allocation fail
        -2: invalid mangled name
        -3: invalid args
    */

    char* demangledName = abi::__cxa_demangle(mangledName, nullptr, nullptr, &demangleStatus);
    if (!demangledName)
        return result;

    result.assign(demangledName);
    std::free(demangledName);

    return result;
}
