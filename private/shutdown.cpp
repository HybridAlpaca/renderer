#include "vk.h"

bool vk_shutdown()
{
	vkDestroyCommandPool(state.device, state.command_pool, nullptr);

	for (const auto & sema : state.image_available_sema)
	{
		vkDestroySemaphore(state.device, sema, nullptr);
	}

	for (const auto & sema : state.render_finished_sema)
	{
		vkDestroySemaphore(state.device, sema, nullptr);
	}

	for (const auto & fence : state.fences)
	{
		vkDestroyFence(state.device, fence, nullptr);
	}

	vkDestroyDevice(state.device, nullptr);

	vkDestroySurfaceKHR(state.instance, state.surface, nullptr);

	vkDestroyInstance(state.instance, nullptr);
}
