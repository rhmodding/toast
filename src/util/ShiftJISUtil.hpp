#ifndef SHIFT_JIS_UTIL
#define SHIFT_JIS_UTIL

#include <cstddef>

#include <string>

namespace ShiftJISUtil {

// Shift-JIS -> UTF8 conversion.
std::string convertToUTF8(const char* string, const size_t stringSize);

// UTF8 -> Shift-JIS conversion.
std::string convertToShiftJIS(const char* string, const size_t stringSize);

} // namespace ShiftJISUtil

#endif // SHIFT_JIS_UTIL
