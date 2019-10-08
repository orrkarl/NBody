#pragma once

#include <cstdlib>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace config
{

constexpr uint32_t WIDTH = 640;
constexpr uint32_t HEIGHT = 480;
constexpr const char* NAME = "triangle";
constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> VALIDATION_LAYERS =
{
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char *> DEVICE_EXTENSIONS = 
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

}