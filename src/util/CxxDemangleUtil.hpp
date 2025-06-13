#ifndef CXX_DEMANGLE_UTIL_HPP
#define CXX_DEMANGLE_UTIL_HPP

#include <string>

#include <typeinfo>

namespace CxxDemangleUtil {

// Returns an empty string if the demangle failed.
[[nodiscard]] std::string Demangle(const char* mangledName);

template <typename T>
[[nodiscard]] std::string Demangle() {
    return Demangle(typeid(T).name());
}

} // namespace CxxDemangleUtil

#endif // CXX_DEMANGLE_UTIL_HPP
