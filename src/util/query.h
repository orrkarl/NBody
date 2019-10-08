#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class SwapChainSupportDetails
{
public:
    SwapChainSupportDetails(const vk::PhysicalDevice& dev, const vk::SurfaceKHR& surface);

	bool isAdequate() const;

    vk::SurfaceFormatKHR chooseFormat() const;

    vk::PresentModeKHR choosePresentMode() const;

    vk::Extent2D chooseExtent(const uint32_t currentWidth, const uint32_t currentHeight) const;

    vk::Extent2D chooseExtent(GLFWwindow* window) const;

    uint32_t chooseImageCount() const;

    vk::SurfaceTransformFlagBitsKHR getCurrentTransform() const;

private:
    const vk::SurfaceCapabilitiesKHR capabilities;
    const std::vector<vk::SurfaceFormatKHR> formats;
    const std::vector<vk::PresentModeKHR> presentModes;
};

std::vector<const char *> getRequiredExtensions();

bool checkDeviceExtensionsSupported(const vk::PhysicalDevice& dev, const std::vector<const char*>& extensions);
