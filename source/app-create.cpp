#include "vk.h"
#include <kernel.h>

#include <glm/glm.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

extern kernel_api * kernel;

static std::vector<char> ReadFile(const char * filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (file.is_open() == false)
	{
		return {};
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

bool vk_app_create()
{
	// Create shader resources
	{
		VkImageCreateInfo traced_image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

		traced_image_info.imageType = VK_IMAGE_TYPE_2D;
		traced_image_info.format    = VK_FORMAT_R8G8B8A8_UNORM;
		traced_image_info.tiling    = VK_IMAGE_TILING_OPTIMAL;
		traced_image_info.usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		traced_image_info.samples   = VK_SAMPLE_COUNT_1_BIT;

		traced_image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
		traced_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		traced_image_info.extent.width  = static_cast<uint32_t>(state.RAYTRACE_RESOLUTION);
		traced_image_info.extent.height = static_cast<uint32_t>(state.RAYTRACE_RESOLUTION);
		traced_image_info.extent.depth  = 1;

		traced_image_info.mipLevels   = 1;
		traced_image_info.arrayLayers = 1;

		vkCreateImage(state.device, &traced_image_info, nullptr, &state.traced_image);

		VkMemoryRequirements traced_image_mem_reqs;
		vkGetImageMemoryRequirements(state.device, state.traced_image, &traced_image_mem_reqs);

		VkMemoryAllocateInfo raytrace_image_alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		raytrace_image_alloc_info.allocationSize  = traced_image_mem_reqs.size;
		raytrace_image_alloc_info.memoryTypeIndex = vk_util_find_memory_type(traced_image_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkAllocateMemory(state.device, &raytrace_image_alloc_info, nullptr, &state.traced_image_mem);

		vkBindImageMemory(state.device, state.traced_image, state.traced_image_mem, 0);

		VkImageViewCreateInfo traced_image_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		traced_image_view_info.image    = state.traced_image;
		traced_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		traced_image_view_info.format   = VK_FORMAT_R8G8B8A8_UNORM;

		traced_image_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		traced_image_view_info.subresourceRange.baseMipLevel   = 0;
		traced_image_view_info.subresourceRange.levelCount     = 1;
		traced_image_view_info.subresourceRange.baseArrayLayer = 0;
		traced_image_view_info.subresourceRange.layerCount     = 1;

		vkCreateImageView(state.device, &traced_image_view_info, nullptr, &state.traced_image_view);
	}

	// Create attachments
	{
		VkImageCreateInfo gbuffer_pos_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

		gbuffer_pos_info.extent = {state.swapchain_extent.width, state.swapchain_extent.height, 1};
		gbuffer_pos_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		gbuffer_pos_info.imageType = VK_IMAGE_TYPE_2D;
		gbuffer_pos_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		gbuffer_pos_info.samples = VK_SAMPLE_COUNT_1_BIT;
		gbuffer_pos_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		gbuffer_pos_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		gbuffer_pos_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		gbuffer_pos_info.mipLevels = 1;
		gbuffer_pos_info.arrayLayers = 1;
		gbuffer_pos_info.flags = 0;

		vkCreateImage(state.device, &gbuffer_pos_info, nullptr, &state.gbuffer_pos);

		VkMemoryRequirements gbuffer_pos_mem_reqs{};

		vkGetImageMemoryRequirements(state.device, state.gbuffer_pos, &gbuffer_pos_mem_reqs);

		VkImageCreateInfo gbuffer_norm_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

		gbuffer_norm_info.extent = {state.swapchain_extent.width, state.swapchain_extent.height, 1};
		gbuffer_norm_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		gbuffer_norm_info.imageType = VK_IMAGE_TYPE_2D;
		gbuffer_norm_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		gbuffer_norm_info.samples = VK_SAMPLE_COUNT_1_BIT;
		gbuffer_norm_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		gbuffer_norm_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		gbuffer_norm_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		gbuffer_norm_info.mipLevels = 1;
		gbuffer_norm_info.arrayLayers = 1;
		gbuffer_norm_info.flags = 0;

		vkCreateImage(state.device, &gbuffer_norm_info, nullptr, &state.gbuffer_norm);

		VkMemoryRequirements gbuffer_norm_mem_reqs{};

		vkGetImageMemoryRequirements(state.device, state.gbuffer_norm, &gbuffer_norm_mem_reqs);

		VkImageCreateInfo gbuffer_albedo_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

		gbuffer_albedo_info.extent = {state.swapchain_extent.width, state.swapchain_extent.height, 1};
		gbuffer_albedo_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		gbuffer_albedo_info.imageType = VK_IMAGE_TYPE_2D;
		gbuffer_albedo_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		gbuffer_albedo_info.samples = VK_SAMPLE_COUNT_1_BIT;
		gbuffer_albedo_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		gbuffer_albedo_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		gbuffer_albedo_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		gbuffer_albedo_info.mipLevels = 1;
		gbuffer_albedo_info.arrayLayers = 1;
		gbuffer_albedo_info.flags = 0;

		vkCreateImage(state.device, &gbuffer_albedo_info, nullptr, &state.gbuffer_albedo);

		VkMemoryRequirements gbuffer_albedo_mem_reqs{};

		vkGetImageMemoryRequirements(state.device, state.gbuffer_albedo, &gbuffer_albedo_mem_reqs);

		VkImageCreateInfo depth_stencil_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

		depth_stencil_info.extent = {state.swapchain_extent.width, state.swapchain_extent.height, 1};
		depth_stencil_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_info.imageType = VK_IMAGE_TYPE_2D;
		depth_stencil_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_stencil_info.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_stencil_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		depth_stencil_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depth_stencil_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		depth_stencil_info.mipLevels = 1;
		depth_stencil_info.arrayLayers = 1;
		depth_stencil_info.flags = 0;

		vkCreateImage(state.device, &depth_stencil_info, nullptr, &state.depth_stencil);

		VkMemoryRequirements depth_stencil_mem_reqs{};

		vkGetImageMemoryRequirements(state.device, state.depth_stencil, &depth_stencil_mem_reqs);

		VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		alloc_info.allocationSize = gbuffer_pos_mem_reqs.size + gbuffer_norm_mem_reqs.size + gbuffer_albedo_mem_reqs.size + depth_stencil_mem_reqs.size;
		alloc_info.memoryTypeIndex = vk_util_find_memory_type(depth_stencil_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkAllocateMemory(state.device, &alloc_info, nullptr, &state.gbuffer_mem);

		vkBindImageMemory(state.device, state.gbuffer_pos, state.gbuffer_mem, 0);
		vkBindImageMemory(state.device, state.gbuffer_norm, state.gbuffer_mem, gbuffer_pos_mem_reqs.size);
		vkBindImageMemory(state.device, state.gbuffer_albedo, state.gbuffer_mem, gbuffer_pos_mem_reqs.size + gbuffer_norm_mem_reqs.size);
		vkBindImageMemory(state.device, state.depth_stencil, state.gbuffer_mem, gbuffer_pos_mem_reqs.size + gbuffer_norm_mem_reqs.size + gbuffer_albedo_mem_reqs.size);

		VkImageViewCreateInfo depth_stencil_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		depth_stencil_view_info.image    = state.depth_stencil;
		depth_stencil_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depth_stencil_view_info.format   = VK_FORMAT_D24_UNORM_S8_UINT;

		depth_stencil_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		depth_stencil_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		depth_stencil_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		depth_stencil_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		depth_stencil_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depth_stencil_view_info.subresourceRange.baseMipLevel   = 0;
		depth_stencil_view_info.subresourceRange.levelCount     = 1;
		depth_stencil_view_info.subresourceRange.baseArrayLayer = 0;
		depth_stencil_view_info.subresourceRange.layerCount     = 1;

		vkCreateImageView(state.device, &depth_stencil_view_info, nullptr, &state.depth_stencil_view);

		VkImageViewCreateInfo gbuffer_pos_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		gbuffer_pos_view_info.image    = state.gbuffer_pos;
		gbuffer_pos_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		gbuffer_pos_view_info.format   = VK_FORMAT_R16G16B16A16_SFLOAT;

		gbuffer_pos_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_pos_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_pos_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_pos_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		gbuffer_pos_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		gbuffer_pos_view_info.subresourceRange.baseMipLevel   = 0;
		gbuffer_pos_view_info.subresourceRange.levelCount     = 1;
		gbuffer_pos_view_info.subresourceRange.baseArrayLayer = 0;
		gbuffer_pos_view_info.subresourceRange.layerCount     = 1;

		vkCreateImageView(state.device, &gbuffer_pos_view_info, nullptr, &state.gbuffer_pos_view);

		VkImageViewCreateInfo gbuffer_norm_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		gbuffer_norm_view_info.image    = state.gbuffer_norm;
		gbuffer_norm_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		gbuffer_norm_view_info.format   = VK_FORMAT_R16G16B16A16_SFLOAT;

		gbuffer_norm_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_norm_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_norm_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_norm_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		gbuffer_norm_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		gbuffer_norm_view_info.subresourceRange.baseMipLevel   = 0;
		gbuffer_norm_view_info.subresourceRange.levelCount     = 1;
		gbuffer_norm_view_info.subresourceRange.baseArrayLayer = 0;
		gbuffer_norm_view_info.subresourceRange.layerCount     = 1;

		vkCreateImageView(state.device, &gbuffer_norm_view_info, nullptr, &state.gbuffer_norm_view);

		VkImageViewCreateInfo gbuffer_albedo_image_view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		gbuffer_albedo_image_view.image    = state.gbuffer_albedo;
		gbuffer_albedo_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		gbuffer_albedo_image_view.format   = VK_FORMAT_R8G8B8A8_UNORM;

		gbuffer_albedo_image_view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_albedo_image_view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_albedo_image_view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		gbuffer_albedo_image_view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		gbuffer_albedo_image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		gbuffer_albedo_image_view.subresourceRange.baseMipLevel   = 0;
		gbuffer_albedo_image_view.subresourceRange.levelCount     = 1;
		gbuffer_albedo_image_view.subresourceRange.baseArrayLayer = 0;
		gbuffer_albedo_image_view.subresourceRange.layerCount     = 1;

		vkCreateImageView(state.device, &gbuffer_albedo_image_view, nullptr, &state.gbuffer_albedo_view);

		VkCommandBufferAllocateInfo cmd_buf_alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buf_alloc_info.commandPool = state.command_pool;
		cmd_buf_alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(state.device, &cmd_buf_alloc_info, &command_buffer);

		VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &beginInfo);
		{
			VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = state.depth_stencil;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			barrier.subresourceRange.baseMipLevel   = 0;
			barrier.subresourceRange.levelCount     = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount     = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			vkCmdPipelineBarrier(
				command_buffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &command_buffer;

		vkQueueSubmit(state.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(state.graphics_queue);

		VkSamplerCreateInfo gbuffer_sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

		gbuffer_sampler_info.magFilter = VK_FILTER_LINEAR;
		gbuffer_sampler_info.minFilter = VK_FILTER_LINEAR;

		gbuffer_sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		gbuffer_sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		gbuffer_sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		gbuffer_sampler_info.anisotropyEnable = VK_FALSE;
		gbuffer_sampler_info.maxAnisotropy    = 1;

		gbuffer_sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		gbuffer_sampler_info.unnormalizedCoordinates = VK_FALSE;

		gbuffer_sampler_info.compareEnable = VK_FALSE;
		gbuffer_sampler_info.compareOp     = VK_COMPARE_OP_ALWAYS;

		gbuffer_sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		gbuffer_sampler_info.mipLodBias = 0.0f;
		gbuffer_sampler_info.minLod     = 0.0f;
		gbuffer_sampler_info.maxLod     = 0.0f;

		vkCreateSampler(state.device, &gbuffer_sampler_info, nullptr, &state.gbuffer_sampler);
	}

	// Create render pass
	{
		// Attachments

		VkAttachmentDescription backbuffer_desc{};
		backbuffer_desc.format  = state.surface_format.format;
		backbuffer_desc.samples = VK_SAMPLE_COUNT_1_BIT;
		backbuffer_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		backbuffer_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		backbuffer_desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		backbuffer_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		backbuffer_desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		backbuffer_desc.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription pos_desc{};
		pos_desc.format  = VK_FORMAT_R16G16B16A16_SFLOAT;
		pos_desc.samples = VK_SAMPLE_COUNT_1_BIT;
		pos_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		pos_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		pos_desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		pos_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		pos_desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		pos_desc.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription norm_desc{};
		norm_desc.format  = VK_FORMAT_R16G16B16A16_SFLOAT;
		norm_desc.samples = VK_SAMPLE_COUNT_1_BIT;
		norm_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		norm_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		norm_desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		norm_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		norm_desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		norm_desc.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription albedo_desc{};
		albedo_desc.format  = VK_FORMAT_R8G8B8A8_UNORM;
		albedo_desc.samples = VK_SAMPLE_COUNT_1_BIT;
		albedo_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		albedo_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		albedo_desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		albedo_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		albedo_desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		albedo_desc.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_desc{};
		depth_desc.format  = VK_FORMAT_D24_UNORM_S8_UINT;
		depth_desc.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_desc.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		const VkAttachmentDescription gbuffer_attachments[]{ pos_desc, norm_desc, albedo_desc, depth_desc };

		// References

		VkAttachmentReference backbuffer_reference{};
		backbuffer_reference.attachment = 0;
		backbuffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference pos_reference{};
		pos_reference.attachment = 0;
		pos_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference norm_reference{};
		norm_reference.attachment = 1;
		norm_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference albedo_reference{};
		albedo_reference.attachment = 2;
		albedo_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_reference{};
		depth_reference.attachment = 3;
		depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		const VkAttachmentReference gbuffer_references[]{ pos_reference, norm_reference, albedo_reference };

		// Subpasses

		VkSubpassDescription gbuffer_subpass_desc{};
		gbuffer_subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		gbuffer_subpass_desc.colorAttachmentCount = 3;
		gbuffer_subpass_desc.pColorAttachments = gbuffer_references;
		gbuffer_subpass_desc.pDepthStencilAttachment = &depth_reference;

		VkSubpassDescription backbuffer_subpass_desc{};
		backbuffer_subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		backbuffer_subpass_desc.colorAttachmentCount = 1;
		backbuffer_subpass_desc.pColorAttachments = &backbuffer_reference;

		// Dependencies

		VkSubpassDependency dependencies[2];

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Render pass

		VkRenderPassCreateInfo gbuffer_pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		gbuffer_pass_info.attachmentCount = 4;
		gbuffer_pass_info.pAttachments    = gbuffer_attachments;
		gbuffer_pass_info.subpassCount    = 1;
		gbuffer_pass_info.pSubpasses      = &gbuffer_subpass_desc;
		gbuffer_pass_info.dependencyCount = 2;
		gbuffer_pass_info.pDependencies   = dependencies;

		VkRenderPassCreateInfo backbuffer_pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		backbuffer_pass_info.attachmentCount = 1;
		backbuffer_pass_info.pAttachments    = &backbuffer_desc;
		backbuffer_pass_info.subpassCount    = 1;
		backbuffer_pass_info.pSubpasses      = &backbuffer_subpass_desc;
		backbuffer_pass_info.dependencyCount = 2;
		backbuffer_pass_info.pDependencies   = dependencies;

		vkCreateRenderPass(state.device, &gbuffer_pass_info, nullptr, &state.gbuffer_pass);
		vkCreateRenderPass(state.device, &backbuffer_pass_info, nullptr, &state.backbuffer_pass);
	}

	// Create framebuffers
	{
		const VkImageView gbuffer_attachments[]{ state.gbuffer_pos_view, state.gbuffer_norm_view, state.gbuffer_albedo_view, state.depth_stencil_view };

		VkFramebufferCreateInfo gbuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

		gbuffer_info.renderPass = state.gbuffer_pass;

		gbuffer_info.attachmentCount = 4;
		gbuffer_info.pAttachments    = gbuffer_attachments;

		gbuffer_info.width  = state.swapchain_extent.width;
		gbuffer_info.height = state.swapchain_extent.height;
		gbuffer_info.layers = 1;

		vkCreateFramebuffer(state.device, &gbuffer_info, nullptr, &state.gbuffer);

		for (unsigned int i = 0; i < state.backbuffer_size; ++i)
		{
			VkFramebufferCreateInfo backbuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

			backbuffer_info.renderPass = state.backbuffer_pass;

			backbuffer_info.attachmentCount = 1;
			backbuffer_info.pAttachments    = &state.backbuffer_views[i];

			backbuffer_info.width  = state.swapchain_extent.width;
			backbuffer_info.height = state.swapchain_extent.height;
			backbuffer_info.layers = 1;

			vkCreateFramebuffer(state.device, &backbuffer_info, nullptr, &state.backbuffers[i]);
		}
	}

	// Create backbuffer output descriptors
	{
		VkDescriptorSetLayoutBinding rendered_image_sampler{};

		rendered_image_sampler.binding    = 0;
		rendered_image_sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		rendered_image_sampler.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		rendered_image_sampler.descriptorCount = 1;

		VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		layout_info.bindingCount = 1;
		layout_info.pBindings    = &rendered_image_sampler;

		vkCreateDescriptorSetLayout(state.device, &layout_info, nullptr, &state.backbuffer_descset_layout);

		VkDescriptorPoolSize pool_size{};

		pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes    = &pool_size;
		pool_info.maxSets       = 1;

		vkCreateDescriptorPool(state.device, &pool_info, nullptr, &state.backbuffer_desc_pool);

		VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		alloc_info.descriptorPool     = state.backbuffer_desc_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts        = &state.backbuffer_descset_layout;

		vkAllocateDescriptorSets(state.device, &alloc_info, &state.backbuffer_descset);

		VkDescriptorImageInfo image_info{};

		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		image_info.imageView = state.traced_image_view;
		image_info.sampler   = state.gbuffer_sampler;

		VkWriteDescriptorSet descriptor_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

		descriptor_write.dstSet = state.backbuffer_descset;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(state.device, 1, &descriptor_write, 0, nullptr);
	}

	// Create tracing descriptors
	{
		VkDescriptorSetLayoutBinding gbuffer_pos_binding{};
		gbuffer_pos_binding.binding = 0;
		gbuffer_pos_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		gbuffer_pos_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		gbuffer_pos_binding.descriptorCount = 1;

		VkDescriptorSetLayoutBinding gbuffer_norm_binding{};
		gbuffer_norm_binding.binding = 1;
		gbuffer_norm_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		gbuffer_norm_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		gbuffer_norm_binding.descriptorCount = 1;

		VkDescriptorSetLayoutBinding gbuffer_albedo_binding{};
		gbuffer_albedo_binding.binding = 2;
		gbuffer_albedo_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		gbuffer_albedo_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		gbuffer_albedo_binding.descriptorCount = 1;

		VkDescriptorSetLayoutBinding rendered_image_binding{};
		rendered_image_binding.binding = 3;
		rendered_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		rendered_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		rendered_image_binding.descriptorCount = 1;

		const VkDescriptorSetLayoutBinding bindings[] { gbuffer_pos_binding, gbuffer_norm_binding, gbuffer_albedo_binding, rendered_image_binding };

		VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		layout_info.bindingCount = 4;
		layout_info.pBindings    = bindings;

		vkCreateDescriptorSetLayout(state.device, &layout_info, nullptr, &state.tracing_descset_layout);

		VkDescriptorPoolSize combined_image_sampler_pool_size{};
		combined_image_sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		combined_image_sampler_pool_size.descriptorCount = 3;

		VkDescriptorPoolSize storage_image_pool_size{};
		storage_image_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storage_image_pool_size.descriptorCount = 1;

		const VkDescriptorPoolSize pool_sizes[] { combined_image_sampler_pool_size, storage_image_pool_size };

		VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

		pool_info.poolSizeCount = 2;
		pool_info.pPoolSizes    = pool_sizes;
		pool_info.maxSets       = 1;

		vkCreateDescriptorPool(state.device, &pool_info, nullptr, &state.tracing_desc_pool);

		VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		alloc_info.descriptorPool     = state.tracing_desc_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts        = &state.tracing_descset_layout;

		vkAllocateDescriptorSets(state.device, &alloc_info, &state.tracing_descset);

		VkDescriptorImageInfo gbuffer_pos_info{};
		gbuffer_pos_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		gbuffer_pos_info.imageView   = state.gbuffer_pos_view;
		gbuffer_pos_info.sampler     = state.gbuffer_sampler;

		VkDescriptorImageInfo gbuffer_norm_info{};
		gbuffer_norm_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		gbuffer_norm_info.imageView   = state.gbuffer_norm_view;
		gbuffer_norm_info.sampler     = state.gbuffer_sampler;

		VkDescriptorImageInfo gbuffer_albedo_info{};
		gbuffer_albedo_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		gbuffer_albedo_info.imageView   = state.gbuffer_albedo_view;
		gbuffer_albedo_info.sampler     = state.gbuffer_sampler;

		VkDescriptorImageInfo traced_image_info{};
		traced_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		traced_image_info.imageView   = state.traced_image_view;
		traced_image_info.sampler     = state.gbuffer_sampler;

		VkWriteDescriptorSet descriptor_writes[4]{};

		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = state.tracing_descset;
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pImageInfo = &gbuffer_pos_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = state.tracing_descset;
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pImageInfo = &gbuffer_norm_info;

		descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[2].dstSet = state.tracing_descset;
		descriptor_writes[2].dstBinding = 2;
		descriptor_writes[2].dstArrayElement = 0;
		descriptor_writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[2].descriptorCount = 1;
		descriptor_writes[2].pImageInfo = &gbuffer_albedo_info;

		descriptor_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[3].dstSet = state.tracing_descset;
		descriptor_writes[3].dstBinding = 3;
		descriptor_writes[3].dstArrayElement = 0;
		descriptor_writes[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptor_writes[3].descriptorCount = 1;
		descriptor_writes[3].pImageInfo = &traced_image_info;

		vkUpdateDescriptorSets(state.device, 4, descriptor_writes, 0, nullptr);
	}

	// Create backbuffer pipeline
	{
		const auto vert_shader_path = std::string(kernel->get_root_dir()) + "assets/shaders/fullscreen.vert.glsl.spv";
		const auto frag_shader_path = std::string(kernel->get_root_dir()) + "assets/shaders/fullscreen.frag.glsl.spv";

		const auto vert_shader_code = ReadFile(vert_shader_path.c_str());
		const auto frag_shader_code = ReadFile(frag_shader_path.c_str());

		VkShaderModuleCreateInfo vert_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		vert_module_info.codeSize = vert_shader_code.size();
		vert_module_info.pCode    = reinterpret_cast<const uint32_t *>(vert_shader_code.data());

		VkShaderModule vert_shader_module;
		vkCreateShaderModule(state.device, &vert_module_info, nullptr, &vert_shader_module);

		VkShaderModuleCreateInfo frag_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		frag_module_info.codeSize = frag_shader_code.size();
		frag_module_info.pCode    = reinterpret_cast<const uint32_t *>(frag_shader_code.data());

		VkShaderModule frag_shader_module;
		vkCreateShaderModule(state.device, &frag_module_info, nullptr, &frag_shader_module);

		VkPipelineShaderStageCreateInfo vert_shader_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		vert_shader_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_info.module = vert_shader_module;
		vert_shader_info.pName  = "main";

		VkPipelineShaderStageCreateInfo frag_shader_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		frag_shader_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_info.module = frag_shader_module;
		frag_shader_info.pName  = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] { vert_shader_info, frag_shader_info };

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) state.swapchain_extent.width;
		viewport.height = (float) state.swapchain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = state.swapchain_extent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
		depth_stencil_state.depthTestEnable = VK_TRUE;
		depth_stencil_state.depthWriteEnable = VK_TRUE;
		depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_state.minDepthBounds = 0.0f;
		depth_stencil_state.maxDepthBounds = 1.0f;
		depth_stencil_state.stencilTestEnable = VK_FALSE;
		depth_stencil_state.front = {};
		depth_stencil_state.back  = {};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPushConstantRange push_constant_range{};

		push_constant_range.offset = 0;
		push_constant_range.size   = sizeof(glm::mat4) * 3;

		push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &state.backbuffer_descset_layout;
		pipeline_layout_info.pushConstantRangeCount = 1;
		pipeline_layout_info.pPushConstantRanges = &push_constant_range;

		vkCreatePipelineLayout(state.device, &pipeline_layout_info, nullptr, &state.backbuffer_pipeline_layout);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shader_stages;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pDepthStencilState = &depth_stencil_state;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = state.backbuffer_pipeline_layout;
		pipelineInfo.renderPass = state.backbuffer_pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state.backbuffer_pipeline);

		vkDestroyShaderModule(state.device, frag_shader_module, nullptr);
		vkDestroyShaderModule(state.device, vert_shader_module, nullptr);
	}

	// Create tracing pipeline
	{
		const auto comp_shader_path = std::string{kernel->get_root_dir()} + "assets/shaders/tracer.comp.glsl.spv";

		const auto comp_shader_code = ReadFile(comp_shader_path.c_str());

        VkShaderModuleCreateInfo comp_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        comp_module_info.codeSize = comp_shader_code.size();
        comp_module_info.pCode    = reinterpret_cast<const uint32_t *>(comp_shader_code.data());

        VkShaderModule comp_shader_module;
        vkCreateShaderModule(state.device, &comp_module_info, nullptr, &comp_shader_module);

		VkPipelineShaderStageCreateInfo compute_shader_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};

		compute_shader_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_info.module = comp_shader_module;
		compute_shader_info.pName  = "main";

		VkPipelineLayoutCreateInfo compute_pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

		compute_pipeline_layout_info.setLayoutCount = 1;
		compute_pipeline_layout_info.pSetLayouts    = &state.tracing_descset_layout;

		vkCreatePipelineLayout(state.device, &compute_pipeline_layout_info, nullptr, &state.tracing_pipeline_layout);

		VkComputePipelineCreateInfo compute_pipeline_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};

		compute_pipeline_info.layout = state.tracing_pipeline_layout;
		compute_pipeline_info.stage  = compute_shader_info;

		vkCreateComputePipelines(state.device, VK_NULL_HANDLE, 1, &compute_pipeline_info, nullptr, &state.tracing_pipeline);

		vkDestroyShaderModule(state.device, comp_shader_module, nullptr);
	}

	return true;
}
