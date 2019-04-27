#include "vk.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

bool vk_frame_begin()
{
	vkWaitForFences(state.device, 1, &state.fences[state.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(state.device, 1, &state.fences[state.current_frame]);

	vkResetCommandBuffer(state.command_bfr_list[state.current_frame], 0);

    vkAcquireNextImageKHR(state.device, state.swapchain, std::numeric_limits<uint64_t>::max(), state.image_available_sema[state.current_frame], VK_NULL_HANDLE, &state.swapchain_image_idx);

	const auto & command_buffer = state.command_bfr_list[state.current_frame];

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	VkRenderPassBeginInfo pass_begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};

	pass_begin_info.renderPass  = state.gbuffer_pass;
	pass_begin_info.framebuffer = state.gbuffer;

	pass_begin_info.renderArea.offset = {0, 0};
	pass_begin_info.renderArea.extent = state.swapchain_extent;

	VkClearValue clear_values[4];

	clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f }; // W value of 0 in position buffer indicates no hit
	clear_values[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clear_values[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clear_values[3].depthStencil = {1.0f, 0};

	pass_begin_info.clearValueCount = 4;
	pass_begin_info.pClearValues    = clear_values;

	vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	return true;
}

bool vk_frame_end()
{
	const auto & command_buffer = state.command_bfr_list[state.current_frame];

	vkCmdEndRenderPass(command_buffer);

	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	barrier.image = state.gbuffer_pos;
	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	barrier.image = state.gbuffer_norm;
	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	barrier.image = state.gbuffer_albedo;
	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.image = state.traced_image;
	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state.tracing_pipeline);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state.tracing_pipeline_layout, 0, 1, &state.tracing_descset, 0, nullptr);

	vkCmdDispatch(command_buffer, state.RAYTRACE_RESOLUTION / 16, state.RAYTRACE_RESOLUTION / 16, 1);

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.image = state.traced_image;
	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	VkRenderPassBeginInfo pass_begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};

	pass_begin_info.renderPass  = state.backbuffer_pass;
	pass_begin_info.framebuffer = state.backbuffers[state.swapchain_image_idx];

	pass_begin_info.renderArea.offset = {0, 0};
	pass_begin_info.renderArea.extent = state.swapchain_extent;

	VkClearValue clear_value = { 0.0f, 1.0f, 0.0f, 1.0f };

	pass_begin_info.clearValueCount = 1;
	pass_begin_info.pClearValues    = &clear_value;

	vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.backbuffer_pipeline);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.backbuffer_pipeline_layout, 0, 1, &state.backbuffer_descset, 0, nullptr);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(command_buffer);

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores    = &state.image_available_sema[state.current_frame];
	submit_info.pWaitDstStageMask  = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer;

	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &state.render_finished_sema[state.current_frame];

	vkQueueSubmit(state.graphics_queue, 1, &submit_info, state.fences[state.current_frame]);

	VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &state.render_finished_sema[state.current_frame];

	present_info.swapchainCount = 1;
	present_info.pSwapchains    = &state.swapchain;
	present_info.pImageIndices  = &state.swapchain_image_idx;

	vkQueuePresentKHR(state.present_queue, &present_info);

	state.current_frame = (state.current_frame + 1) % state.FRAMES_IN_FLIGHT;

	return true;
}

void vk_draw(geometry_o * geo, material_o * mat, const float * model)
{
	const auto & command_buffer = state.command_bfr_list[state.current_frame];

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline);

	VkDeviceSize offsets[]{ 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &geo->vbo, offsets);

	vkCmdBindIndexBuffer(command_buffer, geo->ibo, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->layout, 0, 1, &mat->descset, 0, nullptr);

	const auto model_mat = glm::make_mat4(model);

	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float) state.swapchain_extent.width / (float) state.swapchain_extent.height, 0.1f, 1000.0f);
	proj[1][1] *= -1;

	glm::mat4 frame_data[]{ model_mat, state.view, proj };

	vkCmdPushConstants(command_buffer, mat->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 3, &frame_data);

	vkCmdDrawIndexed(command_buffer, geo->index_count, 1, 0, 0, 0);
}
