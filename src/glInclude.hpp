#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
// GLEW is needed on Windows & Linux; macOS doesn't need it.
#define USING_GLEW
#include <GL/glew.h>

#include <GL/gl.h>
#include <GL/glext.h>
#endif // defined(__APPLE__)
