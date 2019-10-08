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
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;

	bool isAdequate() const
	{
		return !formats.empty() && !presentModes.empty();
	}
};

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return vk::VertexInputBindingDescription(0, sizeof(Vertex));
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription()
	{
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
		};
	}
};

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

const std::vector<Vertex> vertecies
{
	{ { 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    { { 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    { {-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}	
};

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

bool checkDeviceExtensionsSupported(vk::PhysicalDevice dev)
{
	auto properties = dev.enumerateDeviceExtensionProperties();

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

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR renderSurface)
{
	return SwapChainSupportDetails{
		device.getSurfaceCapabilitiesKHR(renderSurface),
		device.getSurfaceFormatsKHR(renderSurface),
		device.getSurfacePresentModesKHR(renderSurface)
	};
}

bool isDeviceSuitable(vk::PhysicalDevice dev, vk::SurfaceKHR renderSurface)
{
	auto queuesFound = findQueueFamilies(dev, renderSurface).isReady();
	auto extensionsSupported = checkDeviceExtensionsSupported(dev);
	auto swapChainAdequate = querySwapChainSupport(dev, renderSurface).isAdequate();
	return queuesFound && extensionsSupported && swapChainAdequate;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
	for (const auto &fmt : availableFormats)
	{
		if (fmt.format == vk::Format::eB8G8R8A8Unorm && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return fmt;
		}
	}

	return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
	for (const auto &availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
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

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
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
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		return extensions;
	}

	uint32_t findMemoryType(const uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags)
	{
		auto memProperties = m_physicalDevice.getMemoryProperties();

		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i)
		{
			if ((typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags))
			{
				return i;
			}
		}

		throw std::runtime_error("could not find compatible memory");
	}

	void createInstance()
	{
		vk::ApplicationInfo appInfo(
			NAME,
			VK_MAKE_VERSION(1, 0, 0),
			"No Engine",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_0
		);

		auto exts = getRequiredExtensions();
		vk::InstanceCreateInfo instInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			VALIDATION_LAYERS.size(), VALIDATION_LAYERS.data(),
			exts.size(), exts.data()
		);

		vk::DebugUtilsMessengerCreateInfoEXT debugInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback, nullptr
		);
		instInfo.pNext = &debugInfo;
		
		m_instance = vk::createInstanceUnique(instInfo);
		m_dispatchDynamic = vk::DispatchLoaderDynamic(m_instance.get(), vkGetInstanceProcAddr);
	}

	void checkValidationLayerSupport()
	{
		auto layers = vk::enumerateInstanceLayerProperties();

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
		vk::DebugUtilsMessengerCreateInfoEXT createInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback, nullptr
		);

		m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr, m_dispatchDynamic);
	}

	void createRenderSurface()
	{
		VkSurfaceKHR surface;
		auto status = glfwCreateWindowSurface(
			static_cast<VkInstance>(m_instance.get()), 
			m_window, 
			nullptr, 
			&surface
		);
		if (status != VK_SUCCESS)
		{
			throw VkError("Could not create render surface", status);
		}

		m_renderSurface = vk::UniqueSurfaceKHR(
			vk::SurfaceKHR(surface), 
			vk::ObjectDestroy(m_instance.get(), nullptr, vk::DispatchLoaderStatic())
		);
	}

	void pickPhysicalDevice()
	{
		auto devices = m_instance->enumeratePhysicalDevices();

		auto deviceCount = devices.size();
		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		for (const auto &device : devices)
		{
			if (isDeviceSuitable(device, m_renderSurface.get()))
			{
				m_physicalDevice = device;
				return;
			}
		}

		throw std::runtime_error("failed to find a suitable GPU!");
	}

	void createLogicalDevice()
	{
		auto indices = findQueueFamilies(m_physicalDevice, m_renderSurface.get());

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		float queuePriority = 1.0f;
		vk::DeviceQueueCreateInfo queueCreateInfo(
			vk::DeviceQueueCreateFlags(),
			0, 
			1, &queuePriority
		);
		for (const auto &family : uniqueQueueFamilies)
		{
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatures;

		vk::DeviceCreateInfo createInfo(
			vk::DeviceCreateFlags(),
			queueCreateInfos.size(), queueCreateInfos.data(),
			VALIDATION_LAYERS.size(), VALIDATION_LAYERS.data(),
			DEVICE_EXTENSIONS.size(), DEVICE_EXTENSIONS.data(),
			&deviceFeatures
		);
		m_device = m_physicalDevice.createDeviceUnique(createInfo);

		m_graphicsQueue = m_device->getQueue(indices.graphicsFamily.value(), 0);
		m_presentQueue = m_device->getQueue(indices.presentFamily.value(), 0);
	}

	void createSwapChain()
	{
		auto support = querySwapChainSupport(m_physicalDevice, m_renderSurface.get());

		auto format = chooseSwapSurfaceFormat(support.formats);
		auto presentationMode = chooseSwapPresentMode(support.presentModes);
		auto extent = chooseSwapExtent(support.capabilities);

		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0)
		{
			imageCount = std::min(imageCount, support.capabilities.maxImageCount);
		}

		vk::SwapchainCreateInfoKHR chainInfo(
			vk::SwapchainCreateFlagsKHR(),
			m_renderSurface.get(),
			imageCount,
			format.format,
			format.colorSpace,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment
		);

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_renderSurface.get());
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		if (indices.graphicsFamily != indices.presentFamily)
		{
			chainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			chainInfo.queueFamilyIndexCount = 2;
			chainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			chainInfo.imageSharingMode = vk::SharingMode::eExclusive;
			chainInfo.queueFamilyIndexCount = 0;	 // Optional
			chainInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		chainInfo.preTransform = support.capabilities.currentTransform;
		chainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		chainInfo.presentMode = presentationMode;
		chainInfo.clipped = VK_TRUE;
		chainInfo.oldSwapchain = vk::SwapchainKHR();

		m_swapChain = m_device->createSwapchainKHRUnique(chainInfo);
		m_swapChainExtent = extent;
		m_swapChainImageFormat = format.format;

		m_swapChainImages = m_device->getSwapchainImagesKHR(m_swapChain.get());
	}

	void createImageViews()
	{
		vk::ImageViewCreateInfo createInfo(
			vk::ImageViewCreateFlags(),
			vk::Image(),
			vk::ImageViewType::e2D,
			m_swapChainImageFormat,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1
			)
		);

		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
		{
			createInfo.image = m_swapChainImages[i];
			m_swapChainImageViews[i] = m_device->createImageViewUnique(createInfo);
		}
	}

	vk::UniqueShaderModule createShaderModule(const std::vector<char> &code)
	{
		vk::ShaderModuleCreateInfo shaderInfo(
			vk::ShaderModuleCreateFlags(),
			code.size(), 
			reinterpret_cast<const uint32_t *>(code.data())
		);

		return m_device->createShaderModuleUnique(shaderInfo);
	}

	void createRenderPass()
	{
		vk::AttachmentDescription color(
			vk::AttachmentDescriptionFlags(),
			m_swapChainImageFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		);

		vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpassDesc;
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &colorRef;

		vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL, 0,
			vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput), vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),
			vk::AccessFlags(), vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
		);

		vk::RenderPassCreateInfo renderPassInfo(
			vk::RenderPassCreateFlags(),
			1, &color,
			1, &subpassDesc,
			1, &dependency
		);

		m_renderPass = m_device->createRenderPassUnique(renderPassInfo);
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("vert.spv");
		auto fragShaderCode = readFile("frag.spv");

		auto fragShader = createShaderModule(fragShaderCode);
		auto vertShader = createShaderModule(vertShaderCode);

		vk::PipelineShaderStageCreateInfo vertInfo(
			vk::PipelineShaderStageCreateFlags(), 
			vk::ShaderStageFlagBits::eVertex, 
			vertShader.get(), 
			"main"
		);

		vk::PipelineShaderStageCreateInfo fragInfo(
			vk::PipelineShaderStageCreateFlags(), 
			vk::ShaderStageFlagBits::eFragment, 
			fragShader.get(), 
			"main"
		);

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertInfo, fragInfo};

		auto bindingDesc = Vertex::getBindingDescription();
		auto attributeDesc = Vertex::getAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInput(
			vk::PipelineVertexInputStateCreateFlags(),
			1, &bindingDesc,
			attributeDesc.size(), attributeDesc.data()
		);

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList,
			VK_FALSE
		);

		vk::Viewport viewport(
			0, 0, 
			static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height),
			0.0f, 1.0f
		);

		vk::Rect2D scissor(
			vk::Offset2D(0, 0), 
			m_swapChainExtent
		);

		vk::PipelineViewportStateCreateInfo viewportState(
			vk::PipelineViewportStateCreateFlags(), 
			1, &viewport, 
			1, &scissor
		);

		vk::PipelineRasterizationStateCreateInfo rasterizerState(
			vk::PipelineRasterizationStateCreateFlags(),
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eClockwise,
			VK_FALSE,
			0.0f, 0.0f, 0.0f,
			1.0f
		);

		vk::PipelineMultisampleStateCreateInfo multisamplingState;

		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.setColorWriteMask(
			vk::ColorComponentFlagBits::eR | 
			vk::ColorComponentFlagBits::eG | 
			vk::ColorComponentFlagBits::eB | 
			vk::ColorComponentFlagBits::eA  
		);

		vk::PipelineColorBlendStateCreateInfo colorBlending;
		colorBlending.setAttachmentCount(1);
		colorBlending.setPAttachments(&colorBlendAttachment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;

		m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

		vk::GraphicsPipelineCreateInfo graphicsInfo(
			vk::PipelineCreateFlags(),
			2, shaderStages,
			&vertexInput,
			&inputAssembly,
			nullptr,
			&viewportState,
			&rasterizerState,
			&multisamplingState,
			nullptr,
			&colorBlending,
			nullptr,
			m_pipelineLayout.get(),
			m_renderPass.get()
		);

		m_pipeline = m_device->createGraphicsPipelineUnique(vk::PipelineCache(), graphicsInfo);
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
		renderPassBegin.renderArea.offset = vk::Offset2D(0, 0);
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

	vk::UniqueBuffer createBuffer(const vk::DeviceSize size, const vk::BufferUsageFlags usage)
	{
		return m_device->createBufferUnique(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),
				size,
				usage
			)
		);
	}

	template <typename T>
	vk::UniqueBuffer createBuffer(const std::vector<T>& data, const vk::BufferUsageFlags usage)
	{
		return createBuffer(sizeof(T) * data.size(), usage);
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
			findMemoryType(
				memRequirements.memoryTypeBits, 
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			)
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

		m_graphicsQueue.submit(vk::ArrayProxy(submitInfo), m_inFlightImages[m_currentFrame].get());

		vk::PresentInfoKHR presentInfo(1, signalSemaphore, 1, &m_swapChain.get(), &imageIndex);

		status = m_presentQueue.presentKHR(presentInfo);
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

// Order of fields is important for destructors
	vk::UniqueInstance 						m_instance;
	vk::UniqueSurfaceKHR 					m_renderSurface;
	vk::UniqueHandle<
		vk::DebugUtilsMessengerEXT, 
		vk::DispatchLoaderDynamic> 			m_debugMessenger;
	vk::UniqueDevice 						m_device;
	vk::UniqueCommandPool 					m_commandPool;
	std::vector<vk::UniqueSemaphore> 		m_imageAvailable;
	std::vector<vk::UniqueFence> 			m_inFlightImages;
	std::vector<vk::UniqueSemaphore>		m_renderCompleted;
	vk::UniqueBuffer						m_vertexBuffer;
	vk::UniqueDeviceMemory					m_vertexDeviceMemory;
	vk::UniqueSwapchainKHR  				m_swapChain;
	std::vector<vk::UniqueImageView> 		m_swapChainImageViews;
	vk::UniqueRenderPass 					m_renderPass;
	vk::UniquePipelineLayout 				m_pipelineLayout;
	vk::UniquePipeline 						m_pipeline;
	std::vector<vk::UniqueCommandBuffer>	m_commandBuffers;
	std::vector<vk::UniqueFramebuffer>		m_frameBuffers;

	int 									m_currentFrame;
	vk::DispatchLoaderDynamic 				m_dispatchDynamic;
	vk::Queue 								m_graphicsQueue;
	vk::PhysicalDevice 						m_physicalDevice;
	vk::Queue 	 							m_presentQueue;
	vk::Extent2D 							m_swapChainExtent;
	vk::Format   							m_swapChainImageFormat;
	std::vector<vk::Image>		 			m_swapChainImages;
	GLFWwindow*								m_window;
	bool									m_windowSizeChanged;

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
