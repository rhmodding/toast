#ifndef CXXDEMANGLE_HPP
#define CXXDEMANGLE_HPP

#include <string>

#include <typeinfo>

namespace CxxDemangle {
    
// Returns an empty string if the demangle failed.
std::string Demangle(const char* mangledName);

template <typename T>
std::string Demangle() {
    return Demangle(typeid(T).name());
}

} // namespace CxxDemangle

#endif // CXXDEMANGLE_HPP
