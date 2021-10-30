#include "vulkan_context.hpp"

HWND create_win32_window(WND_PROC window_proc, uint32_t width, uint32_t height) {
	HINSTANCE instance = GetModuleHandle(NULL);
	WNDCLASS wc{};
	wc.hInstance = instance;
	wc.lpfnWndProc = window_proc;
	wc.lpszClassName = ">_>";

	RegisterClass(&wc);

	HWND window = CreateWindowEx(0, ">_>", "<_<", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, instance, NULL);

	ShowWindow(window, SW_NORMAL);
	return window;
}

void vulkan_context_init(VK_CTX* ctx, HWND window) {
	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo instance_create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instance_create_info.pApplicationInfo = &app_info;

#ifdef EV_DEBUG
	const char* layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_api_dump" };
	instance_create_info.enabledLayerCount = 1;
	instance_create_info.ppEnabledLayerNames = layers;
#endif

	instance_create_info.enabledExtensionCount = 2;
	const char* instance_exts[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	instance_create_info.ppEnabledExtensionNames = instance_exts;

	EV_CHECK_VKRESULT(vkCreateInstance(&instance_create_info, NULL, &ctx->instance));

	uint32_t gpu_count;
	vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, NULL);
	vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, &ctx->gpu_if.gpu);

	float priority = 1.0F;
	VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &priority;
	queue_create_info.queueFamilyIndex = 0;

	VkPhysicalDeviceFeatures features = {};
	VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	device_create_info.pEnabledFeatures = &features;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &queue_create_info;

	device_create_info.enabledExtensionCount = 1;
	const char* device_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	device_create_info.ppEnabledExtensionNames = device_exts;

	EV_CHECK_VKRESULT(vkCreateDevice(ctx->gpu_if.gpu, &device_create_info, NULL, &ctx->gpu_if.device));

	VkWin32SurfaceCreateInfoKHR surface_create_info{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surface_create_info.hwnd = window;
	surface_create_info.hinstance = GetModuleHandle(NULL);

	EV_CHECK_VKRESULT(vkCreateWin32SurfaceKHR(ctx->instance, &surface_create_info, NULL, &ctx->surface));

	VkBool32 present_supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(ctx->gpu_if.gpu, 0, ctx->surface, &present_supported);

	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->gpu_if.gpu, ctx->surface, &caps);
	ctx->present.extent = caps.currentExtent;

	ctx->present.format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	VkSwapchainCreateInfoKHR swapchain_create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_create_info.surface = ctx->surface;
    swapchain_create_info.minImageCount = 2;
    swapchain_create_info.imageFormat = ctx->present.format.format;
    swapchain_create_info.imageColorSpace = ctx->present.format.colorSpace;
    swapchain_create_info.imageExtent = caps.currentExtent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = caps.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.clipped = VK_TRUE;

	EV_CHECK_VKRESULT(vkCreateSwapchainKHR(ctx->gpu_if.device, &swapchain_create_info, NULL, &ctx->present.swapchain));

	vkGetSwapchainImagesKHR(ctx->gpu_if.device, ctx->present.swapchain, &ctx->present.image_count, NULL);
	ctx->present.images = EV_ALLOC(VkImage, ctx->present.image_count);
	ctx->present.views = EV_ALLOC(VkImageView, ctx->present.image_count);
	vkGetSwapchainImagesKHR(ctx->gpu_if.device, ctx->present.swapchain, &ctx->present.image_count, ctx->present.images);

	VkImageViewCreateInfo view_create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	view_create_info.format = ctx->present.format.format;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.layerCount = 1;

	for (uint32_t x = 0; x < ctx->present.image_count; x++) {
		view_create_info.image = ctx->present.images[x];
		EV_CHECK_VKRESULT(vkCreateImageView(ctx->gpu_if.device, &view_create_info, NULL, &ctx->present.views[x]));
	}
}

void create_framebuffers(GpuIF gpu_if, VkRenderPass renderpass, Present_Structure* present) {
	present->framebuffers = EV_ALLOC(VkFramebuffer, present->image_count);
	VkFramebufferCreateInfo framebuffer_create_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebuffer_create_info.attachmentCount = 1;
	framebuffer_create_info.renderPass = renderpass;
	framebuffer_create_info.width = present->extent.width;
	framebuffer_create_info.height = present->extent.height;
	framebuffer_create_info.layers = 1;

	for (uint32_t x = 0; x < present->image_count; x++)
	{
		VkImageView attachments[] = {present->views[x]};
		framebuffer_create_info.pAttachments = attachments;
		EV_CHECK_VKRESULT(vkCreateFramebuffer(gpu_if.device, &framebuffer_create_info, NULL, &present->framebuffers[x]));
	}
}

void vulkan_context_terminate(VK_CTX* ctx) {
	for (uint32_t x = 0; x < ctx->present.image_count; x++) {
		vkDestroyFramebuffer(ctx->gpu_if.device, ctx->present.framebuffers[x], NULL);
		vkDestroyImageView(ctx->gpu_if.device, ctx->present.views[x], NULL);
	}
	EV_FREE(ctx->present.framebuffers);
	EV_FREE(ctx->present.views);
	EV_FREE(ctx->present.images);
	vkDestroySwapchainKHR(ctx->gpu_if.device, ctx->present.swapchain, NULL);
	vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
	vkDestroyDevice(ctx->gpu_if.device, NULL);
	vkDestroyInstance(ctx->instance, NULL);
}

VkCommandPool create_command_pool(GpuIF gpu_if) {
	VkCommandPoolCreateInfo cmd_pool_create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cmd_pool_create_info.queueFamilyIndex = 0;
	cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool cmd_pool;
	EV_CHECK_VKRESULT(vkCreateCommandPool(gpu_if.device, &cmd_pool_create_info, NULL, &cmd_pool));
	return cmd_pool;
}

VkCommandBuffer* allocate_command_buffers(GpuIF gpu_if, VkCommandPool pool, uint32_t count) {
	VkCommandBufferAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = pool;
	allocate_info.commandBufferCount = count;

	VkCommandBuffer* cmd_buffers = EV_ALLOC(VkCommandBuffer, count);
	EV_CHECK_VKRESULT(vkAllocateCommandBuffers(gpu_if.device, &allocate_info, cmd_buffers));
	return cmd_buffers;
}

VkSemaphore create_semaphore(GpuIF gpu_if) {
	VkSemaphoreCreateInfo semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkSemaphore semaphore;
	EV_CHECK_VKRESULT(vkCreateSemaphore(gpu_if.device, &semaphore_create_info, NULL, &semaphore));
	return semaphore;
}

VkFence* create_fences(GpuIF gpu_if, uint32_t count) {
	VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence* fences = EV_ALLOC(VkFence, count);
	for (uint32_t x = 0; x < count; x++)
	{
		EV_CHECK_VKRESULT(vkCreateFence(gpu_if.device, &fence_create_info, NULL, &fences[x]));
	}
	return fences;
}
