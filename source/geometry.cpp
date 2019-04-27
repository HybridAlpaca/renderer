#include "vk.h"

#include <renderer.h>

#include <cstring>

geometry_o * vk_geometry_create(uint32_t vertex_count, vertex_t * vertices, uint32_t index_count, const uint32_t * indices)
{
	auto geo = new geometry_o{};

	VkBufferCreateInfo vertex_buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};

	vertex_buffer_info.size  = sizeof(vertex_t) * vertex_count;
	vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	vertex_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(state.device, &vertex_buffer_info, nullptr, &geo->vbo);

	VkMemoryRequirements vbo_mem_reqs;
	vkGetBufferMemoryRequirements(state.device, geo->vbo, &vbo_mem_reqs);

	VkBufferCreateInfo index_buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};

	index_buffer_info.size  = sizeof(uint32_t) * index_count;
	index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	index_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(state.device, &index_buffer_info, nullptr, &geo->ibo);

	VkMemoryRequirements ibo_mem_reqs;
	vkGetBufferMemoryRequirements(state.device, geo->ibo, &ibo_mem_reqs);

	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize  = vbo_mem_reqs.size + ibo_mem_reqs.size;
	alloc_info.memoryTypeIndex = vk_util_find_memory_type(vbo_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(state.device, &alloc_info, nullptr, &geo->mem);

	vkBindBufferMemory(state.device, geo->vbo, geo->mem, 0);
	vkBindBufferMemory(state.device, geo->ibo, geo->mem, vbo_mem_reqs.size);

	{
		void * data;

		vkMapMemory(state.device, geo->mem, 0, vbo_mem_reqs.size, 0, &data);
		{
			std::memcpy(data, vertices, vertex_buffer_info.size);
		}
		vkUnmapMemory(state.device, geo->mem);
	}

	{
		void * data;

		vkMapMemory(state.device, geo->mem, vbo_mem_reqs.size, ibo_mem_reqs.size, 0, &data);
		{
			std::memcpy(data, indices, index_buffer_info.size);
		}
		vkUnmapMemory(state.device, geo->mem);

		geo->index_count = index_count;
	}

	return geo;
}

void vk_geometry_destroy(const geometry_o * geo)
{
	vkDestroyBuffer(state.device, geo->vbo, nullptr);
	vkDestroyBuffer(state.device, geo->ibo, nullptr);

	vkFreeMemory(state.device, geo->mem, nullptr);

	delete geo;
}

