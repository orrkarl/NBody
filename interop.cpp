#include "interop.h"

#include <glew.h>

#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_OSMESA
#include <GLFW/glfw3native.h>


void ensureInteropAvailable()
{
}

void prepareContextProperties(cl_context_properties properties[7])
{
}
