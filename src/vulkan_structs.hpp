#pragma once
#include <vulkan/vulkan.h>

struct Present_Structure
{
	VkSwapchainKHR swapchain;
	VkImage* images;
	VkImageView* views;
	VkFramebuffer* framebuffers;
	uint32_t image_count;
	VkSurfaceFormatKHR format;
	VkExtent2D extent;
};

struct GpuIF
{
	VkPhysicalDevice gpu;
	VkDevice device;
};

struct VK_CTX
{
	VkInstance instance;
	GpuIF gpu_if;
	VkSurfaceKHR surface;
	Present_Structure present;
};
