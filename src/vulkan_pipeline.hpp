#pragma once

#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "vulkan_structs.hpp"

VkPipelineLayout create_pipeline_layout(GpuIF gpu_if);
VkRenderPass create_renderpass(GpuIF gpu_if);
VkPipeline create_graphics_pipeline(GpuIF gpu_if, VkPipelineLayout layout, VkRenderPass renderpass, const char* vert_spv_file, const char* frag_spv_file);
VkPipeline create_compute_pipeline(GpuIF gpu_if, VkPipelineLayout layout, const char* comp_spv_file);
