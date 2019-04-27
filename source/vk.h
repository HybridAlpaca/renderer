#pragma once

#include <kernel.h>

#include <glm/glm.hpp>

#include "volk/volk.h"

struct GLFWwindow;

struct vertex_t;

struct geometry_o
{
	uint32_t index_count;

	VkBuffer vbo;
	VkBuffer ibo;

	VkDeviceMemory mem;
};

struct material_o
{
	VkPipeline pipeline;

	VkPipelineLayout layout;

	VkImage albedo;

	VkImageView albedo_view;

	VkDeviceMemory mem;

	VkSampler sampler;

	VkDescriptorPool desc_pool;
	VkDescriptorSet descset;
	VkDescriptorSetLayout descset_layout;
};

struct heap_allocation_t
{
	void * data;
	uint64_t size;
};

struct renderer_o
{
	static const uint8_t FRAMES_IN_FLIGHT = 3;

	static const uint32_t MAX_BACKBUFFER_SIZE = 255;

	static const uint32_t RAYTRACE_RESOLUTION = 2048;

	//

	uint32_t backbuffer_size;

	uint8_t current_frame;

	uint32_t swapchain_image_idx;

	//

	GLFWwindow * window;

	glm::mat4 view;

	VkInstance instance;

	VkPhysicalDevice physical_device;

	VkDevice device;

	int32_t graphics_queue_idx;
	int32_t present_queue_idx;

	VkQueue graphics_queue;
	VkQueue present_queue;

	VkRenderPass gbuffer_pass;
	VkRenderPass backbuffer_pass;

	VkPipeline tracing_pipeline;

	VkPipelineLayout tracing_pipeline_layout;

	VkFramebuffer gbuffer;

	VkDescriptorSetLayout backbuffer_descset_layout;
	VkDescriptorSetLayout tracing_descset_layout;

	VkDescriptorSet  backbuffer_descset;
	VkDescriptorPool backbuffer_desc_pool;

	VkDescriptorSet  tracing_descset;
	VkDescriptorPool tracing_desc_pool;

	VkImage gbuffer_pos;
	VkImageView gbuffer_pos_view;

	VkImage gbuffer_norm;
	VkImageView gbuffer_norm_view;

	VkImage gbuffer_albedo;
	VkImageView gbuffer_albedo_view;

	VkImage depth_stencil;
	VkImageView depth_stencil_view;

	VkImage traced_image;
	VkImageView traced_image_view;

	VkSampler gbuffer_sampler;

	VkPipelineLayout backbuffer_pipeline_layout;
	VkPipeline backbuffer_pipeline;

	VkBuffer scene_data_buffer;

	VkDeviceMemory scene_data_buffer_memory;
	VkDeviceMemory traced_image_mem;
	VkDeviceMemory gbuffer_mem;

	VkCommandPool command_pool;
	VkCommandBuffer command_bfr_list[FRAMES_IN_FLIGHT];

	VkSurfaceKHR surface;
	VkSurfaceFormatKHR surface_format;
	VkSwapchainKHR swapchain;
	VkExtent2D swapchain_extent;
	VkPresentModeKHR present_mode;

	VkFramebuffer backbuffers[MAX_BACKBUFFER_SIZE];
	VkImage backbuffer_images[MAX_BACKBUFFER_SIZE];
	VkImageView backbuffer_views[MAX_BACKBUFFER_SIZE];

	VkSemaphore image_available_sema[FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_sema[FRAMES_IN_FLIGHT];
	VkFence fences[FRAMES_IN_FLIGHT];
};

extern renderer_o state;

bool vk_startup();

bool vk_shutdown();

bool vk_app_create();

bool vk_app_destroy();

bool vk_window_create();

bool vk_window_destroy();

bool vk_window_update();

bool vk_swapchain_create();

bool vk_swapchain_destroy();

bool vk_frame_begin();

bool vk_frame_end();

void vk_draw(geometry_o * geo, material_o * mat, const float * model);

uint32_t vk_util_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

heap_allocation_t vk_util_file_data_read(const char * path);

void vk_util_file_data_destroy(const heap_allocation_t data);

geometry_o * vk_geometry_create(uint32_t vertex_count, vertex_t * vertices, uint32_t index_count, const uint32_t * indices);

void vk_geometry_destroy(const geometry_o * geo);

material_o * vk_material_create(void * pixel_data, uint32_t tex_width, uint32_t tex_height);

void vk_material_destroy(const material_o * mat);

void render_component_create(const uint32_t * entities, uint32_t entity_count, uint64_t * instances);

void render_component_lookup(const uint32_t * entities, uint32_t entity_count, uint64_t * instances);

void render_component_destroy(uint32_t * entities, uint32_t entity_count);

void * render_component_get_interface(const char * id);

void render_component_simulate();
