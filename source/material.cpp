/**
 * @file material.cpp
 * @author StickyFingies
 * @brief
 * @version 0.0.1
 * @date 2019-04-21
 *
 * @copyright Copyright (c) 2019
 *
 * @todo Move pipeline creation from here to `app-create.cpp`.  Materials should be nothing more than descriptor sets
 */

#include "vk.h"
#include <renderer.h>

#include <cstring>
#include <iostream>

extern kernel_api * kernel;

material_o * vk_material_create(void * pixel_data, uint32_t tex_width, uint32_t tex_height)
{
	auto mat = new material_o{};

	VkBuffer staging;
	VkDeviceMemory staging_mem;

	VkBufferCreateInfo staging_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	staging_info.size = tex_width * tex_height * 4;
	staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(state.device, &staging_info, nullptr, &staging);

	VkMemoryRequirements staging_mem_reqs;
	vkGetBufferMemoryRequirements(state.device, staging, &staging_mem_reqs);

	VkMemoryAllocateInfo staging_alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	staging_alloc_info.allocationSize  = staging_mem_reqs.size;
	staging_alloc_info.memoryTypeIndex = vk_util_find_memory_type(staging_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(state.device, &staging_alloc_info, nullptr, &staging_mem);

	vkBindBufferMemory(state.device, staging, staging_mem, 0);

	{
		void * data;
		vkMapMemory(state.device, staging_mem, 0, staging_mem_reqs.size, 0, &data);
		{
			memcpy(data, pixel_data, tex_width * tex_height * 4);
		}
		vkUnmapMemory(state.device, staging_mem);
	}

	VkImageCreateInfo albedo_image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	albedo_image_info.imageType = VK_IMAGE_TYPE_2D;
	albedo_image_info.extent.width  = static_cast<uint32_t>(tex_width);
	albedo_image_info.extent.height = static_cast<uint32_t>(tex_height);
	albedo_image_info.extent.depth  = 1;
	albedo_image_info.mipLevels     = 1;
	albedo_image_info.arrayLayers   = 1;
	albedo_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	albedo_image_info.format  = VK_FORMAT_R8G8B8A8_UNORM;
	albedo_image_info.tiling  = VK_IMAGE_TILING_OPTIMAL;
	albedo_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	albedo_image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	albedo_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateImage(state.device, &albedo_image_info, nullptr, &mat->albedo);

	VkMemoryRequirements albedo_mem_reqs;
	vkGetImageMemoryRequirements(state.device, mat->albedo, &albedo_mem_reqs);

	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize  = albedo_mem_reqs.size;
	alloc_info.memoryTypeIndex = vk_util_find_memory_type(albedo_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(state.device, &alloc_info, nullptr, &mat->mem);

	vkBindImageMemory(state.device, mat->albedo, mat->mem, 0);

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
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mat->albedo;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount     = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {tex_width, tex_height, 1};

		vkCmdCopyBufferToImage(
			command_buffer,
			staging,
			mat->albedo,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mat->albedo;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount     = 1;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
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

	vkFreeCommandBuffers(state.device, state.command_pool, 1, &command_buffer);

	vkDestroyBuffer(state.device, staging, nullptr);

	vkFreeMemory(state.device, staging_mem, nullptr);

	VkImageViewCreateInfo albedo_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	albedo_view_info.image = mat->albedo;
	albedo_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	albedo_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	albedo_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	albedo_view_info.subresourceRange.baseMipLevel = 0;
	albedo_view_info.subresourceRange.levelCount = 1;
	albedo_view_info.subresourceRange.baseArrayLayer = 0;
	albedo_view_info.subresourceRange.layerCount = 1;

	vkCreateImageView(state.device, &albedo_view_info, nullptr, &mat->albedo_view);

	VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;

	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy    = 1;

	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	sampler_info.unnormalizedCoordinates = VK_FALSE;

	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp     = VK_COMPARE_OP_ALWAYS;

	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod     = 0.0f;
	sampler_info.maxLod     = 0.0f;

	vkCreateSampler(state.device, &sampler_info, nullptr, &mat->sampler);

	VkDescriptorSetLayoutBinding albedo_sampler_binding{};

	albedo_sampler_binding.binding    = 0;
	albedo_sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	albedo_sampler_binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo_sampler_binding.descriptorCount = 1;

	VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	layout_info.bindingCount = 1;
	layout_info.pBindings    = &albedo_sampler_binding;

	vkCreateDescriptorSetLayout(state.device, &layout_info, nullptr, &mat->descset_layout);

	VkDescriptorPoolSize pool_size{};

	pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes    = &pool_size;
	pool_info.maxSets       = 1;

	vkCreateDescriptorPool(state.device, &pool_info, nullptr, &mat->desc_pool);

	VkDescriptorSetAllocateInfo descset_alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	descset_alloc_info.descriptorPool     = mat->desc_pool;
	descset_alloc_info.descriptorSetCount = 1;
	descset_alloc_info.pSetLayouts        = &mat->descset_layout;

	vkAllocateDescriptorSets(state.device, &descset_alloc_info, &mat->descset);

	VkDescriptorImageInfo image_info{};

	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	image_info.imageView = mat->albedo_view;
	image_info.sampler   = mat->sampler;

	VkWriteDescriptorSet descriptor_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

	descriptor_write.dstSet = mat->descset;
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(state.device, 1, &descriptor_write, 0, nullptr);

	VkVertexInputBindingDescription binding_desc{};
	binding_desc.binding   = 0;
	binding_desc.stride    = sizeof(vertex_t);
	binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrib_descs[3];

	attrib_descs[0].binding  = 0;
	attrib_descs[0].location = 0;
	attrib_descs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attrib_descs[0].offset   = offsetof(vertex_t, pos);

	attrib_descs[1].binding  = 0;
	attrib_descs[1].location = 1;
	attrib_descs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attrib_descs[1].offset   = offsetof(vertex_t, norm);

	attrib_descs[2].binding  = 0;
	attrib_descs[2].location = 2;
	attrib_descs[2].format   = VK_FORMAT_R32G32_SFLOAT;
	attrib_descs[2].offset   = offsetof(vertex_t, uv);

	const auto vert_shader_path = std::string{kernel->get_root_dir()} + "assets/shaders/geometry.vert.glsl.spv";
	const auto frag_shader_path = std::string{kernel->get_root_dir()} + "assets/shaders/geometry.frag.glsl.spv";

	const auto vert_shader_code = vk_util_file_data_read(vert_shader_path.c_str());
	const auto frag_shader_code = vk_util_file_data_read(frag_shader_path.c_str());

	VkShaderModuleCreateInfo vert_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	vert_module_info.codeSize = vert_shader_code.size;
	vert_module_info.pCode    = reinterpret_cast<const uint32_t *>(vert_shader_code.data);

	VkShaderModule vert_shader_module;
	vkCreateShaderModule(state.device, &vert_module_info, nullptr, &vert_shader_module);

	VkShaderModuleCreateInfo frag_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	frag_module_info.codeSize = frag_shader_code.size;
	frag_module_info.pCode    = reinterpret_cast<const uint32_t *>(frag_shader_code.data);

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

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &binding_desc;
	vertexInputInfo.vertexAttributeDescriptionCount = 3;
	vertexInputInfo.pVertexAttributeDescriptions = attrib_descs;

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

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState pos_blend_attachment{};
	pos_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pos_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState norm_blend_attachment{};
	norm_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	norm_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState albedo_blend_attachment{};
	albedo_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	albedo_blend_attachment.blendEnable = VK_FALSE;

	const VkPipelineColorBlendAttachmentState blend_attachments[]{ pos_blend_attachment, norm_blend_attachment, albedo_blend_attachment };

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 3;
	colorBlending.pAttachments = blend_attachments;
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
	pipeline_layout_info.pSetLayouts = &mat->descset_layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	vkCreatePipelineLayout(state.device, &pipeline_layout_info, nullptr, &mat->layout);

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shader_stages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pDepthStencilState = &depth_stencil_state;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = mat->layout;
	pipelineInfo.renderPass = state.gbuffer_pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mat->pipeline);

	vkDestroyShaderModule(state.device, frag_shader_module, nullptr);
	vkDestroyShaderModule(state.device, vert_shader_module, nullptr);

	vk_util_file_data_destroy(vert_shader_code);
	vk_util_file_data_destroy(frag_shader_code);

	return mat;
}

void vk_material_destroy(const material_o * mat)
{
	vkDestroyPipeline(state.device, mat->pipeline, nullptr);
	vkDestroyPipelineLayout(state.device, mat->layout, nullptr);

	vkDestroyDescriptorPool(state.device, mat->desc_pool, nullptr);
	vkDestroyDescriptorSetLayout(state.device, mat->descset_layout, nullptr);

	vkDestroySampler(state.device, mat->sampler, nullptr);

	vkDestroyImageView(state.device, mat->albedo_view, nullptr);

	vkDestroyImage(state.device, mat->albedo, nullptr);

	vkFreeMemory(state.device, mat->mem, nullptr);

	delete mat;
}
