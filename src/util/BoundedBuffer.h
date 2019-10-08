#pragma once

#include <vulkan/vulkan.hpp>

class BoundedBuffer
{
public:
    

private:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
};
