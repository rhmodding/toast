#ifndef CXXDEMANGLE_HPP
#define CXXDEMANGLE_HPP

#include <string>

namespace CxxDemangle {
    
// Returns an empty string if the demangle failed.
std::string Demangle(const char* mangledName);

} // namespace CxxDemangle

#endif // CXXDEMANGLE_HPP
