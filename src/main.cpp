#include "utils.hpp"

#include "vulkan_context.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_resource.hpp"

static bool running;
static uint32_t width = 1400;
static uint32_t height = 900;
LRESULT CALLBACK window_proc(HWND window, UINT msg, WPARAM w_param, LPARAM l_param) {
	switch (msg) {
		case WM_DESTROY: {
			running = false;
			PostQuitMessage(0);
		}
	}
	return DefWindowProc(window, msg, w_param, l_param);
}

void handle_message(MSG* msg) {
	while (PeekMessage(msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(msg);
		DispatchMessage(msg);
	}
}

struct vec3 { float x, y, z; };
struct Vertex { vec3 pos; };

int main() {
	MSG msg = {}; 
	VK_CTX vk_ctx;
	HWND window = create_win32_window(window_proc, width, height);
	vulkan_context_init(&vk_ctx, window);

	VkRenderPass renderpass = create_renderpass(vk_ctx.gpu_if);
	create_framebuffers(vk_ctx.gpu_if, renderpass, &vk_ctx.present);
	uint32_t image_count = vk_ctx.present.image_count;

	BufferBlock block0 = create_bufferblock(vk_ctx.gpu_if, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	BufferBlock block1 = create_bufferblock(vk_ctx.gpu_if, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	VkDeviceSize size = 128 * 1024 * 1024;
	VkDeviceMemory host_visible_chuck = allocate_memory(vk_ctx.gpu_if, size, block0.requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkBindBufferMemory(vk_ctx.gpu_if.device, block0.buffer, host_visible_chuck, 0);
	vkBindBufferMemory(vk_ctx.gpu_if.device, block1.buffer, host_visible_chuck, block0.requirements.size);

	VkPipelineLayout pipeline_layout = create_pipeline_layout(vk_ctx.gpu_if);
	VkPipeline gfx_pipeline = create_graphics_pipeline(vk_ctx.gpu_if, pipeline_layout, renderpass,
		"shaders//test.vert.spv", "shaders/test.frag.spv");

	VkCommandPool cmd_pool = create_command_pool(vk_ctx.gpu_if);
	VkCommandBuffer* cmd_buffers = allocate_command_buffers(vk_ctx.gpu_if, cmd_pool, image_count);

	uint32_t scr_width = vk_ctx.present.extent.width;
	uint32_t scr_height = vk_ctx.present.extent.height;
	VkCommandBufferBeginInfo cmd_buffer_begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkRenderPassBeginInfo renderpass_begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderpass_begin_info.renderPass = renderpass;
	renderpass_begin_info.renderArea = { {0, 0}, {scr_width, scr_height} };
	renderpass_begin_info.clearValueCount = 1;
	VkClearValue clear_value = {{0.0, 0.0, 0.0, 1.0}};
	renderpass_begin_info.pClearValues = &clear_value;

	VkRect2D scissor = { {0, 0}, vk_ctx.present.extent };
	VkViewport viewport = {0, (float)scr_height, (float)scr_width, -(float)scr_height, 0.0, 1.0};
	for (uint32_t x = 0; x < image_count; x++) {
		renderpass_begin_info.framebuffer = vk_ctx.present.framebuffers[x];
		EV_CHECK_VKRESULT(vkBeginCommandBuffer(cmd_buffers[x], &cmd_buffer_begin_info));

		vkCmdBeginRenderPass(cmd_buffers[x], &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(cmd_buffers[x], 0, 1, &viewport);
		vkCmdSetScissor(cmd_buffers[x], 0, 1, &scissor);

		vkCmdBindPipeline(cmd_buffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, gfx_pipeline);

		vkCmdDraw(cmd_buffers[x], 3, 1, 0, 0);

		vkCmdEndRenderPass(cmd_buffers[x]);
		vkEndCommandBuffer(cmd_buffers[x]);
	}

	VkSemaphore image_acquired_sem = create_semaphore(vk_ctx.gpu_if);
	VkSemaphore render_complete_sem = create_semaphore(vk_ctx.gpu_if);
	VkFence* wait_fences = create_fences(vk_ctx.gpu_if, image_count);

	VkQueue queue;
	vkGetDeviceQueue(vk_ctx.gpu_if.device, 0, 0, &queue);

	uint32_t img_ix;
	running = true;
	while (running) {
		handle_message(&msg);

		EV_CHECK_VKRESULT(vkAcquireNextImageKHR(vk_ctx.gpu_if.device, vk_ctx.present.swapchain, INT64_MAX, image_acquired_sem, VK_NULL_HANDLE, &img_ix));

		EV_CHECK_VKRESULT(vkWaitForFences(vk_ctx.gpu_if.device, 1, &wait_fences[img_ix], VK_TRUE, INT64_MAX));
		EV_CHECK_VKRESULT(vkResetFences(vk_ctx.gpu_if.device, 1, &wait_fences[img_ix]));

		VkPipelineStageFlags wait_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submit_info.pWaitDstStageMask = wait_mask;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &image_acquired_sem;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &render_complete_sem;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &cmd_buffers[img_ix];
		EV_CHECK_VKRESULT(vkQueueSubmit(queue, 1, &submit_info, wait_fences[img_ix]));

		VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &vk_ctx.present.swapchain;
		present_info.pImageIndices = &img_ix;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &render_complete_sem;
		EV_CHECK_VKRESULT(vkQueuePresentKHR(queue, &present_info));
	}
	vkDeviceWaitIdle(vk_ctx.gpu_if.device);

	vkDestroySemaphore(vk_ctx.gpu_if.device, image_acquired_sem, NULL);
	vkDestroySemaphore(vk_ctx.gpu_if.device, render_complete_sem, NULL);
	for (uint32_t x = 0; x < image_count; x++) {
		vkDestroyFence(vk_ctx.gpu_if.device, wait_fences[x], NULL);
	}
	vkDestroyPipelineLayout(vk_ctx.gpu_if.device, pipeline_layout, NULL);
	vkDestroyPipeline(vk_ctx.gpu_if.device, gfx_pipeline, NULL);
	vkDestroyRenderPass(vk_ctx.gpu_if.device, renderpass, NULL);
	vkDestroyCommandPool(vk_ctx.gpu_if.device, cmd_pool, NULL);
	vulkan_context_terminate(&vk_ctx);
}