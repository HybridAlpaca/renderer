#include "vk.h"
#include <renderer.h>
#include <entity.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cstring>
#include <iostream>
#include <map>
#include <vector>

struct render_component_instance_t
{
	const geometry_o * geo;
	const material_o * mat;
};

struct render_component_o
{
	std::vector<render_component_instance_t> instances;

	std::map<uint32_t, uint64_t> entity_instance_map;
};

static render_component_o rc_state{};

static void render_component_i_set_primitives(uint64_t instance, const geometry_o * geo, const material_o * mat)
{
	rc_state.instances[instance].geo = geo;
	rc_state.instances[instance].mat = mat;
}

static render_component_i render_component
{
	render_component_i_set_primitives
};

void render_component_create(const uint32_t * entities, uint32_t entity_count, uint64_t * instances)
{
	for (uint32_t i{0}; i < entity_count; ++i)
	{
		rc_state.entity_instance_map[entities[i]] = rc_state.instances.size();
		instances[i] = rc_state.instances.size();

		rc_state.instances.emplace_back(render_component_instance_t{});
	}
}

void render_component_lookup(const uint32_t * entities, uint32_t entity_count, uint64_t * instances)
{
	for (uint32_t i{0}; i < entity_count; ++i)
	{
		instances[i] = rc_state.entity_instance_map[entities[i]];
	}
}

void render_component_destroy(uint32_t * entities, uint32_t entity_count)
{
	for (uint32_t i{0}; i < entity_count; ++i)
	{
		const uint32_t instance = rc_state.entity_instance_map[entities[i]];
		const uint32_t last_instance = rc_state.instances.size() - 1;

		rc_state.instances[instance] = rc_state.instances[last_instance];

		rc_state.instances.pop_back();
	}
}

void * render_component_get_interface(const char * id)
{
	if (std::strcmp(id, RENDER_COMPONENT_INTERFACE_NAME) == 0)
	{
		return &render_component;
	}

	return nullptr;
}

void render_component_simulate()
{
	vk_frame_begin();

	for (const auto & inst : rc_state.instances)
	{
		const auto & command_buffer = state.command_bfr_list[state.current_frame];

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, inst.mat->pipeline);

		VkDeviceSize offsets[]{ 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &inst.geo->vbo, offsets);

		vkCmdBindIndexBuffer(command_buffer, inst.geo->ibo, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, inst.mat->layout, 0, 1, &inst.mat->descset, 0, nullptr);

		const auto model_mat = glm::mat4(1.0f);

		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float) state.swapchain_extent.width / (float) state.swapchain_extent.height, 0.1f, 1000.0f);
		proj[1][1] *= -1;

		glm::mat4 frame_data[]{ model_mat, state.view, proj };

		vkCmdPushConstants(command_buffer, inst.mat->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 3, &frame_data);

		vkCmdDrawIndexed(command_buffer, inst.geo->index_count, 1, 0, 0, 0);
	}

	vk_frame_end();
};
