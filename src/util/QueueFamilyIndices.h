#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

class QueueFamilyIndices
{
public:
    QueueFamilyIndices(const vk::PhysicalDevice& dev, const vk::SurfaceKHR& surface);

    uint32_t graphics() const;

    uint32_t present() const;

    bool hasAllQueues() const;
    
    bool hasGraphics() const;

    bool hasPresent() const;

    operator bool() const;

private:
    std::optional<uint32_t> m_graphics = std::nullopt;
    std::optional<uint32_t> m_present = std::nullopt;
};
