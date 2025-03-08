// HACK: needed to supress warning at stb_image_write.h:776
#define __STDC_LIB_EXT1__
#define sprintf_s snprintf

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
