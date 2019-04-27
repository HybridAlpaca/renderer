#pragma once

#include <stdint.h>

static const char * RENDERER_API_NAME = "renderer";
static const char * RENDER_COMPONENT_INTERFACE_NAME = "render_component_i";

struct geometry_o;
struct material_o;
struct component_i;

struct vertex_t
{
	float pos[3];
	float norm[3];
	float uv[2];
};

struct renderer_api
{
	geometry_o * (*geometry_create)(uint32_t vertex_count, vertex_t * vertices, uint32_t index_count, const uint32_t * indices);

	void (*geometry_destroy)(const geometry_o * geo);

	material_o * (*material_create)(void * pixel_data, uint32_t tex_width, uint32_t tex_height);

	void (*material_destroy)(const material_o * mat);

	void (*draw)(geometry_o * geo, material_o * mat, const float * model);

	void (*wait)();

	component_i * (*get_component_manager)();
};

struct render_component_i
{
	void (*set_primitives)(uint64_t instance, const geometry_o * geo, const material_o * mat);
};
