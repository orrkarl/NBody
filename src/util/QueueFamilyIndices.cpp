#include "QueueFamilyIndices.h"

QueueFamilyIndices::QueueFamilyIndices(const vk::PhysicalDevice& dev, const vk::SurfaceKHR& surface)
{
	auto families = dev.getQueueFamilyProperties();
	vk::Bool32 presentSupport = false;

	uint32_t queueIdx = 0;
	for (const auto& family : families)
	{
		if (family.queueCount > 0)
		{
			if (family.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				m_graphics = queueIdx;
			}

			presentSupport = dev.getSurfaceSupportKHR(queueIdx, surface);
			if (presentSupport)
			{
				m_present = queueIdx;
			}
		}

		queueIdx++;
		if (hasAllQueues())
		{
			break;
		}
	}
}

uint32_t QueueFamilyIndices::graphics() const
{
    return *m_graphics;
}

uint32_t QueueFamilyIndices::present() const
{
    return *m_present;
}

bool QueueFamilyIndices::hasAllQueues() const
{
    return hasGraphics() and hasPresent();
}

bool QueueFamilyIndices::hasGraphics() const
{
    return m_graphics.has_value();
}

bool QueueFamilyIndices::hasPresent() const
{
    return m_present.has_value();
}

QueueFamilyIndices::operator bool() const
{
    return hasAllQueues();
}