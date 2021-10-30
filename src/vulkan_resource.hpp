#pragma once

#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "vulkan_structs.hpp"

struct BufferBlock
{
	VkBuffer buffer;
	VkMemoryRequirements requirements;
	VkDeviceMemory memory;
};

struct ImageBlock
{
	VkImage image;
	VkMemoryRequirements requirements;
	VkDeviceMemory memory;
};

VkDeviceMemory allocate_memory(GpuIF gpu_if, VkDeviceSize size, uint32_t required_memory_bits, VkMemoryPropertyFlags properties);
BufferBlock create_bufferblock(GpuIF gpu_if, VkDeviceSize size, VkBufferUsageFlags usage);
void destroy_bufferblock(GpuIF gpu_if, BufferBlock block);
ImageBlock create_imageblock(GpuIF gpu_if, VkExtent3D extent, VkImageUsageFlags usage);
void destroy_imageblock(GpuIF gpu_if, ImageBlock block);