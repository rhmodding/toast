#if defined(__WIN32__) || defined(__linux__)
#define USING_GLEW
#include <GL/glew.h>
#endif // defined(__WIN32__) || defined(__linux__)

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif // defined(__APPLE__)
