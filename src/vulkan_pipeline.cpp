#include "vulkan_pipeline.hpp"

VkShaderModule read_SPIRV(GpuIF gpu_if, const char* filename) {
	size_t size;
	FILE* fptr;
	fopen_s(&fptr, filename, "rb");

	fseek(fptr, 0, SEEK_END);
	size = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	char* code = EV_ALLOC(char, size);
	size_t result = fread(code, sizeof(char), size, fptr);
	fclose(fptr);

	VkShaderModuleCreateInfo module_create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	module_create_info.codeSize = size;
	module_create_info.pCode = (uint32_t*)code;

	VkShaderModule shader_module;
	EV_CHECK_VKRESULT(vkCreateShaderModule(gpu_if.device, &module_create_info, NULL, &shader_module));
	EV_FREE(code);
	return shader_module;
}

VkRenderPass create_renderpass(GpuIF gpu_if) {
	VkAttachmentDescription color_attachment = {};
    color_attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_ref = {};
	color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_ref.attachment = 0;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_ref;
	subpass.pDepthStencilAttachment = 0;
	subpass.pResolveAttachments = 0;

	VkRenderPassCreateInfo renderpass_create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderpass_create_info.attachmentCount = 1;
	renderpass_create_info.pAttachments = &color_attachment;
	renderpass_create_info.subpassCount = 1;
	renderpass_create_info.pSubpasses = &subpass;
	renderpass_create_info.dependencyCount = 0;
	renderpass_create_info.pDependencies = 0;

	VkRenderPass renderpass;
	EV_CHECK_VKRESULT(vkCreateRenderPass(gpu_if.device, &renderpass_create_info, NULL, &renderpass));
	return renderpass;
}

VkPipelineLayout create_pipeline_layout(GpuIF gpu_if) {
	VkPipelineLayoutCreateInfo layout_create_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layout_create_info.setLayoutCount = 0;
	layout_create_info.pSetLayouts = 0;
	layout_create_info.pushConstantRangeCount = 0;
	layout_create_info.pPushConstantRanges = 0;

	VkPipelineLayout layout;
	EV_CHECK_VKRESULT(vkCreatePipelineLayout(gpu_if.device, &layout_create_info, NULL, &layout));
	return layout;
}

VkPipeline create_graphics_pipeline(GpuIF gpu_if, VkPipelineLayout layout, VkRenderPass renderpass, const char* vert_spv_file, const char* frag_spv_file) {
	VkShaderModule vert_module = read_SPIRV(gpu_if, vert_spv_file);
	VkShaderModule frag_module = read_SPIRV(gpu_if, frag_spv_file);

	VkPipelineShaderStageCreateInfo vertex_stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_stage.module = vert_module;
	vertex_stage.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_stage.module = frag_module;
	fragment_stage.pName = "main";

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	vertex_input_state.vertexAttributeDescriptionCount = 0;
	vertex_input_state.pVertexAttributeDescriptions = 0;
	vertex_input_state.vertexBindingDescriptionCount = 0;
	vertex_input_state.pVertexBindingDescriptions = 0;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewport_state = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport_state.scissorCount = 1;
	viewport_state.viewportCount = 1;

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates = states;

	VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0F;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo multisample_state = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_attachment_state = {};
	color_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT ;

	VkPipelineColorBlendStateCreateInfo color_blend_state = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &color_attachment_state;

	VkPipelineShaderStageCreateInfo stages[] = { vertex_stage, fragment_stage };
	VkGraphicsPipelineCreateInfo gfx_pipeline_create_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	gfx_pipeline_create_info.layout = layout;
	gfx_pipeline_create_info.renderPass = renderpass;
	gfx_pipeline_create_info.subpass = 0;
	gfx_pipeline_create_info.stageCount = 2;
	gfx_pipeline_create_info.pStages = stages;
	gfx_pipeline_create_info.pVertexInputState = &vertex_input_state;
	gfx_pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	gfx_pipeline_create_info.pViewportState = &viewport_state;
	gfx_pipeline_create_info.pRasterizationState = &rasterizer;
	gfx_pipeline_create_info.pMultisampleState = &multisample_state;
	gfx_pipeline_create_info.pDepthStencilState = NULL;
	gfx_pipeline_create_info.pDynamicState = &dynamic_state;
	gfx_pipeline_create_info.pColorBlendState = &color_blend_state;

	VkPipeline pipeline;
	EV_CHECK_VKRESULT(vkCreateGraphicsPipelines(gpu_if.device, NULL, 1, &gfx_pipeline_create_info, NULL, &pipeline));
	vkDestroyShaderModule(gpu_if.device, vert_module, NULL);
	vkDestroyShaderModule(gpu_if.device, frag_module, NULL);
	return pipeline;
}

VkPipeline create_compute_pipeline(GpuIF gpu_if, VkPipelineLayout layout, const char* comp_spv_file) {
	VkShaderModule comp_module = read_SPIRV(gpu_if, comp_spv_file);
	VkPipelineShaderStageCreateInfo stage_create_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	stage_create_info.module = comp_module;
	stage_create_info.pName = "main";
	stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;

	VkComputePipelineCreateInfo comp_pipeline_create_info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	comp_pipeline_create_info.layout = layout;
	comp_pipeline_create_info.stage = stage_create_info;

	VkPipeline pipeline;
	EV_CHECK_VKRESULT(vkCreateComputePipelines(gpu_if.device, NULL, 1, &comp_pipeline_create_info, NULL, &pipeline));
	vkDestroyShaderModule(gpu_if.device, comp_module, NULL);
	return pipeline;
}