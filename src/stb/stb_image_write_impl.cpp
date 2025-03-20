// HACK: needed to supress warning at stb_image_write.h:776
#if defined(__GNUC__)

#define __STDC_LIB_EXT1__

#if !defined(sprintf_s)

#include <stdio.h>
#include <stdarg.h>

static inline int sprintf_s(char *str, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}

#endif // !defined(sprintf_s)

#endif // defined(__GNUC__)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
