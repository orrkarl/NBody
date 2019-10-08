#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

void onKeyPress(GLFWwindow* window, const int key, const int scancode, const int action, const int mods);

void onGLFWError(const int error, const char* description);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT flags,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *userData
);
