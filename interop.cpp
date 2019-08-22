#include "interop.h"

#include <glew.h>

#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_GLX
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WGL
#else
#error "Unsupported OS, I'm sorry :("
#endif

#include <GLFW/glfw3native.h>

void prepareContextProperties(const cl::Platform& platform, cl_context_properties properties[7]);

void ensureDeviceInteropCapability(const cl::Device& dev, const cl_context_properties* properties);

void prepareInterop(const cl::Platform& platform, const cl::Device& dev, cl_context_properties properties[7])
{

	prepareContextProperties(platform, properties);
	ensureDeviceInteropCapability(dev, properties);
}


void prepareContextProperties(const cl::Platform& platform, cl_context_properties properties[7])
{
#if defined(__linux__)
	properties[0] = CL_GL_CONTEXT_KHR;
	properties[1] = (cl_context_properties)glXGetCurrentContext();
	properties[2] = CL_GLX_DISPLAY_KHR;
	properties[3] = (cl_context_properties)glXGetCurrentDisplay();
	properties[4] = CL_CONTEXT_PLATFORM;
	properties[5] = (cl_context_properties)platform;
	properties[6] = 0;
#elif defined(_WIN32)
	properties[0] = CL_GL_CONTEXT_KHR;
	properties[1] = (cl_context_properties)wglGetCurrentContext();
	properties[2] = CL_WGL_HDC_KHR;
	properties[3] = (cl_context_properties)wglGetCurrentDC();
	properties[4] = CL_CONTEXT_PLATFORM;
	properties[5] = (cl_context_properties)platform();
	properties[6] = 0;
#endif
}

void ensureDeviceInteropCapability(const cl::Device& dev, const cl_context_properties* properties)
{
	cl_device_id devices[32]{ nullptr };
	size_t size;
	unsigned int count;
	cl_int err;
	auto id = dev();

	err = clGetGLContextInfoKHR(properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, sizeof(devices), devices, &size);
	if (err != CL_SUCCESS)
	{
		throw cl::Error(err, "clGetGLContextInfoKHR");
	}

	count = size / sizeof(cl_device_id);

	for (auto i = 0u; i < count; ++i)
	{
		if (devices[i] == id)
		{
			return;
		}
	}

	throw std::runtime_error("Chosen device has no interop capabilities");
}
