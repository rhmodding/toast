#ifndef CXX_DEMANGLE_HPP
#define CXX_DEMANGLE_HPP

#include <string>

#include <typeinfo>

namespace CxxDemangle {

// Returns an empty string if the demangle failed.
[[nodiscard]] std::string Demangle(const char* mangledName);

template <typename T>
[[nodiscard]] std::string Demangle() {
    return Demangle(typeid(T).name());
}

} // namespace CxxDemangle

#endif // CXX_DEMANGLE_HPP
