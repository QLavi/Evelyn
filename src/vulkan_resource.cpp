#include "vulkan_resource.hpp"

uint32_t find_memory_index(VkPhysicalDevice gpu, uint32_t required_memory_bits, VkMemoryPropertyFlags required_memory_properties) {
	VkPhysicalDeviceMemoryProperties gpu_memory_properties;
	vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_memory_properties);

	uint32_t memory_count = gpu_memory_properties.memoryTypeCount;
	for (uint32_t x = 0; x < memory_count; x++)
	{
		uint32_t memory_bits = (1 << x);
		VkMemoryPropertyFlags properties = gpu_memory_properties.memoryTypes[x].propertyFlags;

		bool is_required_memory_type = required_memory_bits & memory_bits;
		bool has_required_properties = (properties & required_memory_properties) == required_memory_properties;

		if (is_required_memory_type && has_required_properties) return x;
	}
	return -1;
}

VkDeviceMemory allocate_memory(GpuIF gpu_if, VkDeviceSize size, uint32_t required_memory_bits, VkMemoryPropertyFlags properties) {
	VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocate_info.allocationSize = size;

	uint32_t ix = find_memory_index(gpu_if.gpu, required_memory_bits, properties);
	allocate_info.memoryTypeIndex = ix;

	VkDeviceMemory memory;
	EV_CHECK_VKRESULT(vkAllocateMemory(gpu_if.device, &allocate_info, NULL, &memory));
	return memory;
}

BufferBlock create_bufferblock(GpuIF gpu_if, VkDeviceSize size, VkBufferUsageFlags usage) {
	BufferBlock block = {};
	VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	EV_CHECK_VKRESULT(vkCreateBuffer(gpu_if.device, &buffer_create_info, NULL, &block.buffer));

	vkGetBufferMemoryRequirements(gpu_if.device, block.buffer, &block.requirements);

	return block;
}
void destroy_bufferblock(GpuIF gpu_if, BufferBlock block) {
	vkDestroyBuffer(gpu_if.device, block.buffer, NULL);
	vkFreeMemory(gpu_if.device, block.memory, NULL);
}

ImageBlock create_imageblock(GpuIF gpu_if, VkExtent3D extent, VkImageUsageFlags usage) {
	ImageBlock block = {};
	VkImageCreateInfo image_create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent = extent;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	EV_CHECK_VKRESULT(vkCreateImage(gpu_if.device, &image_create_info, NULL, &block.image));

	vkGetImageMemoryRequirements(gpu_if.device, block.image, &block.requirements);

	return block;
}
void destroy_imageblock(GpuIF gpu_if, ImageBlock block) {
	vkDestroyImage(gpu_if.device, block.image, NULL);
	vkFreeMemory(gpu_if.device, block.memory, NULL);
}
