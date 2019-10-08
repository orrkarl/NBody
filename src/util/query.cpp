#include "query.h"

#include <algorithm>

#include "general.h"


SwapChainSupportDetails::SwapChainSupportDetails(const vk::PhysicalDevice& dev, const vk::SurfaceKHR& surface)
    : 
    capabilities(dev.getSurfaceCapabilitiesKHR(surface)), 
    formats(dev.getSurfaceFormatsKHR(surface)), 
    presentModes(dev.getSurfacePresentModesKHR(surface))
{
}

bool SwapChainSupportDetails::isAdequate() const
{
    return !formats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR SwapChainSupportDetails::chooseFormat() const
{
    for (const auto &fmt : formats)
	{
		if (fmt.format == vk::Format::eB8G8R8A8Unorm && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return fmt;
		}
	}

	return formats[0];
}

vk::PresentModeKHR SwapChainSupportDetails::choosePresentMode() const
{
    for (const auto &availablePresentMode : presentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChainSupportDetails::chooseExtent(const uint32_t currentWidth, const uint32_t currentHeight) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    return {
        clamp(capabilities.minImageExtent.width, currentWidth, capabilities.maxImageExtent.width),
        clamp(capabilities.minImageExtent.height, currentHeight, capabilities.maxImageExtent.height)
    };
}

vk::Extent2D SwapChainSupportDetails::chooseExtent(GLFWwindow* window) const
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
    return chooseExtent(w, h);
}

uint32_t SwapChainSupportDetails::chooseImageCount() const
{
    uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0)
	{
		imageCount = std::min(imageCount, capabilities.maxImageCount);
	}
    return imageCount;
}

vk::SurfaceTransformFlagBitsKHR SwapChainSupportDetails::getCurrentTransform() const
{
    return capabilities.currentTransform;
}

std::vector<const char *> getRequiredExtensions()
{
    uint32_t extCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + extCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

bool checkDeviceExtensionsSupported(const vk::PhysicalDevice& dev, const std::vector<const char*>& extensions)
{
	auto properties = dev.enumerateDeviceExtensionProperties();

	bool isFound = false;	
	for (const auto &extName : extensions)
	{
		for (const auto &extProperties : properties)
		{
			if (!strcmp(extName, extProperties.extensionName))
			{
				isFound = true;
				break;
			}
		}

		if (!isFound)
		{
			return false;
		}
	}

	return true;
}
