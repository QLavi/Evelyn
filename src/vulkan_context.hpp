#pragma once

#undef UNICODE
#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "vulkan_structs.hpp"

typedef LRESULT CALLBACK WND_PROC(HWND, UINT, WPARAM, LPARAM);
HWND create_win32_window(WND_PROC window_proc, uint32_t width, uint32_t height);
void vulkan_context_init(VK_CTX* ctx, HWND window);
void vulkan_context_terminate(VK_CTX* ctx);
void create_framebuffers(GpuIF gpu_if, VkRenderPass renderpass, Present_Structure* present);
VkCommandPool create_command_pool(GpuIF gpu_if);
VkCommandBuffer* allocate_command_buffers(GpuIF gpu_if, VkCommandPool pool, uint32_t count);
VkSemaphore create_semaphore(GpuIF gpu_if);
VkFence* create_fences(GpuIF gpu_if, uint32_t count);
