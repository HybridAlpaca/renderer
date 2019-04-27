#include <renderer.h>
#include <kernel.h>
#include <entity.h>
#include "vk.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

renderer_o state{};

component_i render_component
{
	render_component_create,
	render_component_lookup,
	render_component_destroy,

	render_component_get_interface
};

render_i renderer
{
	vk_geometry_create,
	vk_geometry_destroy,

	vk_material_create,
	vk_material_destroy,

	vk_draw,

	[](){ vkDeviceWaitIdle(state.device); },
	[](){ return &render_component; }
};

kernel_i * kernel;

bool start(kernel_i * api)
{
	kernel = api;

	kernel->api_impl_add(RENDER_INTERFACE_NAME, &renderer);
	kernel->api_impl_add(COMPONENT_INTERFACE_NAME, &render_component);

	vk_window_create();

	vk_startup();

	vk_swapchain_create();

	vk_app_create();

	return true;
}

bool stop()
{
	std::cout << "Stopping renderer" << std::endl;

	vkDeviceWaitIdle(state.device);

	std::cout << "renderer idle" << std::endl;

	vk_app_destroy();
	vk_swapchain_destroy();
	vk_shutdown();
	vk_window_destroy();

	return true;
}

bool update()
{
	glfwPollEvents();

	if (glfwWindowShouldClose(state.window))
	{
		kernel->shutdown();
	}

	vk_window_update();

	render_component_simulate();

	glfwSwapBuffers(state.window);

	return true;
}

const char * exports[]{ RENDER_INTERFACE_NAME };

extern "C" const module_desc_t module
{
	"renderer",

	exports,
	nullptr,

	1,
	0,

	start,
	stop,
	update
};
