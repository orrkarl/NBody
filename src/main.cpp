#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "util/debug.h"

void onKeyPress(GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

void onGLFWError(const int error, const char* description)
{
	std::cerr << description << '(' << error << ")\n";
}

class VkError : public std::runtime_error
{
public:
	VkError(const char *msg, const VkResult errcode)
		: std::runtime_error(msg), status(errcode)
	{
	}

	const VkResult status;
};

class VkLayerNotFoundError : public std::exception
{
public:
	VkLayerNotFoundError(const char *layerName)
		: std::exception(), m_layerName(layerName), m_errorMessage(std::string("Validation layer not found: ") + layerName)
	{
	}

	const char *what() const noexcept override
	{
		return m_errorMessage.c_str();
	}

	const char *layer() const
	{
		return m_layerName;
	}

private:
	const char *m_layerName;
	const std::string m_errorMessage;
};

class VkExtensionNotFoundError : public std::runtime_error
{
public:
	VkExtensionNotFoundError(const char *extensionName)
		: std::runtime_error(std::string("Validation layer not found: ") + extensionName), m_extensionName(extensionName)
	{
	}

	const char *extensions() const
	{
		return m_extensionName;
	}

private:
	const char *m_extensionName;
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isReady() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	bool isAdequate() const
	{
		return !formats.empty() && !presentModes.empty();
	}
};

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = { };
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription()
	{
		VkVertexInputAttributeDescription pos = { };
		pos.binding = 0;
		pos.format = VK_FORMAT_R32G32_SFLOAT;
		pos.location = 0;
		pos.offset = offsetof(Vertex, pos);

		VkVertexInputAttributeDescription color = { };
		color.binding = 0;
		color.format = VK_FORMAT_R32G32B32_SFLOAT;
		color.location = 1;
		color.offset = offsetof(Vertex, color);

		return { pos, color };
	}
};

constexpr uint32_t WIDTH = 640;
constexpr uint32_t HEIGHT = 480;
constexpr const char *NAME = "triangle";
constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> VALIDATION_LAYERS =
{
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char *> DEVICE_EXTENSIONS = 
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<Vertex> vertecies
{
	{ { 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    { { 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    { {-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}	
};

std::vector<char> readFile(const std::string &path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error(std::string("could not open shader file: ") + path);
	}

	auto fileSize = file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR renderSurface)
{
	QueueFamilyIndices indices;

	auto families = device.getQueueFamilyProperties();
	VkBool32 presentSupport = false;

	uint32_t queueIdx = 0;
	for (const auto& family : families)
	{
		if (family.queueCount > 0)
		{
			if (family.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = queueIdx;
			}

			presentSupport = device.getSurfaceSupportKHR(queueIdx, renderSurface);
			if (presentSupport)
			{
				indices.presentFamily = queueIdx;
			}
		}

		queueIdx++;
		if (indices.isReady())
		{
			break;
		}
	}

	return indices;
}

bool checkDeviceExtensionsSupported(VkPhysicalDevice dev)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> properties(extensionCount);
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, properties.data());

	bool isFound = false;
	for (const auto &extName : DEVICE_EXTENSIONS)
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

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR renderSurface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, renderSurface, &details.capabilities);

	uint32_t count;

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderSurface, &count, nullptr);
	if (count != 0)
	{
		details.formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderSurface, &count, details.formats.data());
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderSurface, &count, nullptr);
	if (count != 0)
	{
		details.presentModes.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderSurface, &count, details.presentModes.data());
	}

	return details;
}

bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR renderSurface)
{
	auto queuesFound = findQueueFamilies(dev, renderSurface).isReady();
	auto extensionsSupported = checkDeviceExtensionsSupported(dev);
	auto swapChainAdequate = querySwapChainSupport(dev, renderSurface).isAdequate();
	return queuesFound && extensionsSupported && swapChainAdequate;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	for (const auto &fmt : availableFormats)
	{
		if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return fmt;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
	for (const auto &availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

template <typename T>
T clamp(const T &min, const T &value, const T &max)
{
	return std::max(min, std::min(value, max));
}

class HelloTriangleApp
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	static void glfwFramebufferResize(GLFWwindow* window, int w, int h)
	{
		auto app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
		app->m_windowSizeChanged = true;
	}

	void initWindow()
	{
		glfwInit();
		glfwSetErrorCallback(&onGLFWError);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, &onKeyPress);
		glfwSetFramebufferSizeCallback(m_window, &glfwFramebufferResize);
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int w, h;
		glfwGetFramebufferSize(m_window, &w, &h);

		return {
			clamp(capabilities.minImageExtent.width, static_cast<uint32_t>(w), capabilities.maxImageExtent.width),
			clamp(capabilities.minImageExtent.height, static_cast<uint32_t>(h), capabilities.maxImageExtent.height)
		};
	}

	std::vector<const char *> getRequiredExtensions()
	{
		uint32_t extCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + extCount);
		if (ENABLE_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	uint32_t findMemoryType(const uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags == propertyFlags))
			{
				return i;
			}
		}

		throw std::runtime_error("could not find compatible memory");
	}

	void createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = NAME;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instInfo = {};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pApplicationInfo = &appInfo;
		auto exts = getRequiredExtensions();
		instInfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
		instInfo.ppEnabledExtensionNames = exts.data();

		if (ENABLE_VALIDATION_LAYERS)
		{
			instInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			instInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
			populateDebugMessengerCreateInfo(debugInfo);
			instInfo.pNext = &debugInfo;
		}
		else
		{
			instInfo.enabledLayerCount = 0;
			instInfo.pNext = nullptr;
		}

		VkResult status = vkCreateInstance(&instInfo, nullptr, &m_instance);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create instance!", status);
		}
	}

	void checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> layers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

		for (auto layerName : VALIDATION_LAYERS)
		{
			bool isFound = false;
			for (const auto &layerProperties : layers)
			{
				if (!strcmp(layerName, layerProperties.layerName))
				{
					isFound = true;
					break;
				}
			}

			if (!isFound)
			{
				throw VkLayerNotFoundError(layerName);
			}
		}
	}

	void setupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		populateDebugMessengerCreateInfo(createInfo);
		auto status = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, m_debugMessenger);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create debug messenger", status);
		}
	}

	void createRenderSurface()
	{
		auto status = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_renderSurface);
		if (status != VK_SUCCESS)
		{
			throw VkError("Could not create render surface", status);
		}
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for (const auto &device : devices)
		{
			if (isDeviceSuitable(device, m_renderSurface))
			{
				m_physicalDevice = device;
				break;
			}
		}

		if (m_physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice()
	{
		auto indices = findQueueFamilies(m_physicalDevice, m_renderSurface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		for (const auto &family : uniqueQueueFamilies)
		{
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());

		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		

		auto status = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create a logical device", status);
		}

		m_dispatchDynamic = vk::DispatchLoaderDynamic(m_instance.get(), vkGetInstanceProcAddr, m_device.get(), vkGetDeviceProcAddr);

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	void createSwapChain()
	{
		auto support = querySwapChainSupport(m_physicalDevice, m_renderSurface);

		auto format = chooseSwapSurfaceFormat(support.formats);
		auto presentationMode = chooseSwapPresentMode(support.presentModes);
		auto extent = chooseSwapExtent(support.capabilities);

		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0)
		{
			imageCount = std::min(imageCount, support.capabilities.maxImageCount);
		}

		VkSwapchainCreateInfoKHR chainInfo = {};
		chainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		chainInfo.surface = m_renderSurface;
		chainInfo.minImageCount = imageCount;
		chainInfo.imageFormat = format.format;
		chainInfo.imageColorSpace = format.colorSpace;
		chainInfo.imageExtent = extent;
		chainInfo.imageArrayLayers = 1;
		chainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_renderSurface);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		if (indices.graphicsFamily != indices.presentFamily)
		{
			chainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			chainInfo.queueFamilyIndexCount = 2;
			chainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			chainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			chainInfo.queueFamilyIndexCount = 0;	 // Optional
			chainInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		chainInfo.preTransform = support.capabilities.currentTransform;
		chainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		chainInfo.presentMode = presentationMode;
		chainInfo.clipped = VK_TRUE;
		chainInfo.oldSwapchain = VK_NULL_HANDLE;

		auto status = vkCreateSwapchainKHR(m_device, &chainInfo, nullptr, &m_swapChain);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create swap chain", status);
		}

		m_swapChainExtent = extent;
		m_swapChainImageFormat = format.format;

		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
	}

	void createImageViews()
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult status;
		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
		{
			createInfo.image = m_swapChainImages[i];
			status = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);
			if (status != VK_SUCCESS)
			{
				throw VkError("could not create image view", status);
			}
		}
	}

	VkShaderModule createShaderModule(const std::vector<char> &code)
	{
		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.codeSize = code.size();
		shaderInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		VkShaderModule ret;
		auto status = vkCreateShaderModule(m_device, &shaderInfo, nullptr, &ret);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create shader module", status);
		}

		return ret;
	}

	void createRenderPass()
	{
		VkAttachmentDescription color = {};
		color.format = m_swapChainImageFormat;
		color.samples = VK_SAMPLE_COUNT_1_BIT;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorRef = {};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDesc = {};
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &colorRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &color;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDesc;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		auto status = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create render pass", status);
		}
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("vert.spv");
		auto fragShaderCode = readFile("frag.spv");

		auto fragShader = createShaderModule(fragShaderCode);
		auto vertShader = createShaderModule(vertShaderCode);

		VkPipelineShaderStageCreateInfo vertInfo = {};
		vertInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertInfo.module = vertShader;
		vertInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragInfo = {};
		fragInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragInfo.module = fragShader;
		fragInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertInfo, fragInfo};

		auto bindingDesc = Vertex::getBindingDescription();
		auto attributeDesc = Vertex::getAttributeDescription();

		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &bindingDesc;
		vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
		vertexInput.pVertexAttributeDescriptions = attributeDesc.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerState = {};
		rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerState.depthClampEnable = VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = VK_FALSE;
		rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerState.lineWidth = 1.0f;
		rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizerState.depthBiasClamp = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisamplingState = {};
		multisamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingState.sampleShadingEnable = VK_FALSE;
		multisamplingState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;			  // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr;		  // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0;	// Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		auto status = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create pipeline layout!", status);
		}

		VkGraphicsPipelineCreateInfo graphicsInfo = {};
		graphicsInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsInfo.stageCount = 2;
		graphicsInfo.pStages = shaderStages;
		graphicsInfo.pVertexInputState = &vertexInput;
		graphicsInfo.pInputAssemblyState = &inputAssembly;
		graphicsInfo.pViewportState = &viewportState;
		graphicsInfo.pRasterizationState = &rasterizerState;
		graphicsInfo.pMultisampleState = &multisamplingState;
		graphicsInfo.pColorBlendState = &colorBlending;
		graphicsInfo.layout = m_pipelineLayout;
		graphicsInfo.renderPass = m_renderPass;
		graphicsInfo.subpass = 0;

		status = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &graphicsInfo, nullptr, &m_pipeline);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create graphics pipeline", status);
		}

		vkDestroyShaderModule(m_device, vertShader, nullptr);
		vkDestroyShaderModule(m_device, fragShader, nullptr);
	}

	void createFramebuffers()
	{
		m_frameBuffers.resize(m_swapChainImages.size());

		vk::FramebufferCreateInfo framebufferInfo(
			vk::FramebufferCreateFlags(), 
			m_renderPass.get(), 
			1, nullptr, 
			m_swapChainExtent.width, m_swapChainExtent.height, 
			1
		);

		for (auto i = 0u; i < m_swapChainImageViews.size(); ++i)
		{
			framebufferInfo.pAttachments = &m_swapChainImageViews[i].get();
			m_frameBuffers[i] = m_device->createFramebufferUnique(framebufferInfo);
		}
	}

	void createCommandPool()
	{
		auto indices = findQueueFamilies(m_physicalDevice, m_renderSurface.get());
		vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), indices.graphicsFamily.value());
		m_commandPool = m_device->createCommandPoolUnique(commandPoolInfo);
	}

	void createCommandBuffers()
	{
		vk::CommandBufferAllocateInfo allocInfo(
			m_commandPool.get(), 
			vk::CommandBufferLevel::ePrimary, 
			static_cast<uint32_t>(m_swapChainImageViews.size())
		);

		m_commandBuffers = m_device->allocateCommandBuffersUnique(allocInfo);

		vk::CommandBufferBeginInfo commandBufferBegin{};

		vk::Buffer vertexBuffers[] = { m_vertexBuffer.get() };
		vk::DeviceSize vertexOffsets[] = { 0 };

		vk::RenderPassBeginInfo renderPassBegin;
		renderPassBegin.renderPass = m_renderPass.get();
		renderPassBegin.renderArea.offset = {0, 0};
		renderPassBegin.renderArea.extent = m_swapChainExtent;
		vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
		renderPassBegin.clearValueCount = 1;
		renderPassBegin.pClearValues = &clearColor;

		for (auto i = 0u; i < m_swapChainImageViews.size(); ++i)
		{
			renderPassBegin.framebuffer = m_frameBuffers[i].get();

			m_commandBuffers[i]->begin(commandBufferBegin);
				m_commandBuffers[i]->beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
				m_commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
				m_commandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, vertexOffsets);
				m_commandBuffers[i]->draw(static_cast<uint32_t>(vertecies.size()), 1, 0, 0);
				m_commandBuffers[i]->endRenderPass();
			m_commandBuffers[i]->end();
		}
	}

	void createSyncObjects()
	{
		auto size = m_swapChainImages.size();

		m_imageAvailable.resize(size);
		m_renderCompleted.resize(size);
		m_inFlightImages.resize(size);

		auto semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlags(vk::FenceCreateFlagBits::eSignaled));

		for (auto i = 0u; i < size; ++i)
		{
			m_imageAvailable[i] = m_device->createSemaphoreUnique(semaphoreInfo);
			m_renderCompleted[i] = m_device->createSemaphoreUnique(semaphoreInfo);
			m_inFlightImages[i] = m_device->createFenceUnique(fenceInfo);
		}
	}

	void recreateSwapchain()
	{
		int w = 0, h = 0;
		while (w == 0 && h == 0)
		{
			glfwGetFramebufferSize(m_window, &w, &h);
			glfwWaitEvents();
		}

		m_device->waitIdle();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	void createVertexBuffer()
	{
		vk::BufferCreateInfo bufferInfo(
			vk::BufferCreateFlags(), 
			sizeof(vertecies[0]) * vertecies.size(), 
			vk::BufferUsageFlags(vk::BufferUsageFlagBits::eVertexBuffer)
		);

		m_vertexBuffer = m_device->createBufferUnique(bufferInfo);
		
		auto memRequirements = m_device->getBufferMemoryRequirements(m_vertexBuffer.get());
		
		vk::MemoryAllocateInfo allocInfo(
			memRequirements.size, 
			findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		);

		m_vertexDeviceMemory = m_device->allocateMemoryUnique(allocInfo);	

		void* data = m_device->mapMemory(m_vertexDeviceMemory.get(), 0, bufferInfo.size);		
		std::memcpy(data, vertecies.data(), static_cast<size_t>(bufferInfo.size));
		m_device->unmapMemory(m_vertexDeviceMemory.get());

		m_device->bindBufferMemory(m_vertexBuffer.get(), m_vertexDeviceMemory.get(), 0);
	}

	void initVulkan()
	{
		createInstance();
		
		checkValidationLayerSupport();
		setupDebugMessenger();
		
		createRenderSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createVertexBuffer();
		createCommandBuffers();
		createSyncObjects();
	}

	void drawFrame()
	{
		m_device->waitForFences(1, &m_inFlightImages[m_currentFrame].get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
		const vk::Semaphore* waitSemaphore = &m_imageAvailable[m_currentFrame].get();
		const vk::Semaphore* signalSemaphore = &m_renderCompleted[m_currentFrame].get();

		uint32_t imageIndex;
		auto status = m_device->acquireNextImageKHR(m_swapChain.get(), std::numeric_limits<uint64_t>::max(), m_imageAvailable[m_currentFrame].get(), vk::Fence(), &imageIndex);
		if (status == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapchain();
			return;
		}
		
		else if (status != vk::Result::eSuccess && status != vk::Result::eSuboptimalKHR)
		{
			vk::throwResultException(status, "could not aquire next image");
		}

		const vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo(1, waitSemaphore, &waitStage, 1, &m_commandBuffers[imageIndex].get(), 1, signalSemaphore);

		m_device->resetFences(1, &m_inFlightImages[m_currentFrame].get());

		m_graphicsQueue.submit(vk::ArrayProxy(submitInfo), m_inFlightImages[m_currentFrame].get(), m_dispatchStatic);

		vk::PresentInfoKHR presentInfo(1, signalSemaphore, 1, &m_swapChain.get(), &imageIndex);

		status = m_presentQueue.presentKHR(presentInfo, m_dispatchStatic);
		if (status == vk::Result::eErrorOutOfDateKHR || status == vk::Result::eSuboptimalKHR)
		{
			recreateSwapchain();
		}

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			drawFrame();
			glfwPollEvents();
		}

		m_graphicsQueue.waitIdle();
		m_presentQueue.waitIdle();
	}

	void cleanup()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	std::vector<vk::UniqueCommandBuffer>	m_commandBuffers;
	vk::UniqueCommandPool 					m_commandPool;
	int 									m_currentFrame;
	vk::UniqueDebugUtilsMessengerEXT 		m_debugMessenger;
	vk::UniqueDevice 						m_device;
	std::vector<vk::UniqueFramebuffer>		m_frameBuffers;
	vk::Queue 								m_graphicsQueue;
	std::vector<vk::UniqueSemaphore> 		m_imageAvailable;
	std::vector<vk::UniqueFence> 			m_inFlightImages;
	vk::UniqueInstance 						m_instance;
	vk::UniquePipeline 						m_pipeline;
	vk::PhysicalDevice 						m_physicalDevice;
	vk::UniquePipelineLayout 				m_pipelineLayout;
	vk::Queue 	 							m_presentQueue;
	vk::UniqueRenderPass 					m_renderPass;
	std::vector<vk::UniqueSemaphore>		m_renderCompleted;
	vk::UniqueSurfaceKHR 					m_renderSurface;
	vk::UniqueSwapchainKHR  				m_swapChain;
	vk::Extent2D 							m_swapChainExtent;
	vk::Format   							m_swapChainImageFormat;
	std::vector<vk::UniqueImage> 			m_swapChainImages;
	std::vector<vk::UniqueImageView> 		m_swapChainImageViews;
	vk::UniqueBuffer						m_vertexBuffer;
	vk::UniqueDeviceMemory					m_vertexDeviceMemory;
	GLFWwindow*								m_window;
	bool									m_windowSizeChanged;

	vk::DispatchLoaderStatic m_dispatchStatic = vk::DispatchLoaderStatic();
	vk::DispatchLoaderDynamic m_dispatchDynamic;
};

int main()
{
	auto app = HelloTriangleApp();

	try
	{
		app.run();
	}
	catch (const VkError &ex)
	{
		std::cerr << "Error while running app: " << ex.what() << "(" << ex.status << ")\n";
		return EXIT_FAILURE;
	}
	catch (const std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
