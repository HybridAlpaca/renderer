#include "vk.h"

bool vk_app_destroy()
{
	vkDestroyDescriptorPool(state.device, state.backbuffer_desc_pool, nullptr);
	vkDestroyDescriptorPool(state.device, state.tracing_desc_pool, nullptr);

	vkDestroyDescriptorSetLayout(state.device, state.backbuffer_descset_layout, nullptr);
	vkDestroyDescriptorSetLayout(state.device, state.tracing_descset_layout, nullptr);

	vkDestroyRenderPass(state.device, state.gbuffer_pass, nullptr);
	vkDestroyRenderPass(state.device, state.backbuffer_pass, nullptr);

	vkDestroyPipeline(state.device, state.backbuffer_pipeline, nullptr);
	vkDestroyPipeline(state.device, state.tracing_pipeline, nullptr);

	vkDestroyPipelineLayout(state.device, state.backbuffer_pipeline_layout, nullptr);
	vkDestroyPipelineLayout(state.device, state.tracing_pipeline_layout, nullptr);

	vkDestroyFramebuffer(state.device, state.gbuffer, nullptr);

	for (uint16_t i{0}; i < state.backbuffer_size; ++i)
	{
		vkDestroyFramebuffer(state.device, state.backbuffers[i], nullptr);
	}

	vkDestroyImageView(state.device, state.gbuffer_pos_view, nullptr);
	vkDestroyImageView(state.device, state.gbuffer_norm_view, nullptr);
	vkDestroyImageView(state.device, state.gbuffer_albedo_view, nullptr);
	vkDestroyImageView(state.device, state.depth_stencil_view, nullptr);
	vkDestroyImageView(state.device, state.traced_image_view, nullptr);

	vkDestroyImage(state.device, state.gbuffer_pos, nullptr);
	vkDestroyImage(state.device, state.gbuffer_norm, nullptr);
	vkDestroyImage(state.device, state.gbuffer_albedo, nullptr);
	vkDestroyImage(state.device, state.depth_stencil, nullptr);
	vkDestroyImage(state.device, state.traced_image, nullptr);

	vkFreeMemory(state.device, state.gbuffer_mem, nullptr);
	vkFreeMemory(state.device, state.traced_image_mem, nullptr);
	vkFreeMemory(state.device, state.scene_data_buffer_memory, nullptr);

	vkDestroySampler(state.device, state.gbuffer_sampler, nullptr);

	return true;
}
