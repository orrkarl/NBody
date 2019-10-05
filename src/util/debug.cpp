#include <iostream>

#include "debug.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT flags,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *userData)
{
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cerr << "ERROR: " << data->pMessage << std::endl;
	}
}

