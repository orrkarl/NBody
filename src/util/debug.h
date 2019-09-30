#pragma once

#include <vulkan/vulkan.hpp>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT flags,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *userData
);

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &info);

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT &messenger
);

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT &messenger
);
