#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "vk.h"

#include <GLFW/glfw3.h>

#include <cstring>
#include <fstream>
#include <vector>

static const bool debug{true};

bool vk_startup()
{
	volkInitialize();

	//////
	// Create Instance
	//////

	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app_info.pApplicationName   = "unknown";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName        = "render-backend-vk";
	app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion         = VK_API_VERSION_1_0;

	const char * extension_names[]
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		#ifdef WIN32
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		#else
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
		#endif
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instanceInfo.pApplicationInfo        = &app_info;
	instanceInfo.enabledExtensionCount   = sizeof(extension_names) / sizeof(extension_names[0]);
	instanceInfo.ppEnabledExtensionNames = extension_names;

	if (debug)
	{
		const char * layer_names[]
		{
			"VK_LAYER_LUNARG_standard_validation"
		};

		instanceInfo.enabledLayerCount   = sizeof(layer_names) / sizeof(layer_names[0]);
		instanceInfo.ppEnabledLayerNames = layer_names;
	}

	if (vkCreateInstance(&instanceInfo, nullptr, &state.instance) != VK_SUCCESS)
	{
		return false;
	}

	volkLoadInstance(state.instance);

	//////
	// Create Surface
	//////

	if (glfwCreateWindowSurface(state.instance, state.window, nullptr, &state.surface) != VK_SUCCESS)
	{
		return false;
	}

	//////
	// Choose Physical Device
	//////

	uint32_t available_device_count;
	vkEnumeratePhysicalDevices(state.instance, &available_device_count, nullptr);

	if (available_device_count == 0)
	{
		return false;
	}

	VkPhysicalDevice available_devices[available_device_count];
	vkEnumeratePhysicalDevices(state.instance, &available_device_count, available_devices);

	const char * required_extensions[]
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	for (const auto & device : available_devices)
	{
		uint32_t available_extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, nullptr);
		VkExtensionProperties available_extensions[available_extension_count];
		vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, available_extensions);

		uint32_t supported_extension_count{0};

		for (const auto & required_ext : required_extensions)
		{
			for (const auto & available_ext : available_extensions)
			{
				if (strcmp(required_ext, available_ext.extensionName) == 0)
				{
					++supported_extension_count;
					break;
				}
			}
		}

		if (supported_extension_count != (sizeof(required_extensions) / sizeof(required_extensions[0])))
		{
			continue;
		}

		state.physical_device = device;
	}

	if (state.physical_device == VK_NULL_HANDLE)
	{
		return false;
	}

	//////
	// Create Logical Device
	//////

	uint32_t queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(state.physical_device, &queue_family_count, nullptr);
	VkQueueFamilyProperties queue_families[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(state.physical_device, &queue_family_count, queue_families);

	int32_t queue_idx{0};
	for (const auto & family : queue_families)
	{
		if (family.queueCount == 0)
		{
			continue;
		}

		if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			state.graphics_queue_idx = queue_idx;
		}

		VkBool32 present_support{false};
		vkGetPhysicalDeviceSurfaceSupportKHR(state.physical_device, queue_idx, state.surface, &present_support);

		if (present_support)
		{
			state.present_queue_idx = queue_idx;
		}

		if (state.graphics_queue_idx && state.present_queue_idx)
		{
			break;
		}

		++queue_idx;
	}

	const float queue_priority{1.0f};

	VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};

	queue_info.queueFamilyIndex = state.graphics_queue_idx;
	queue_info.queueCount       = 1;
	queue_info.pQueuePriorities = &queue_priority;

	const char * required_device_extensions[]
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkPhysicalDeviceFeatures device_features{};

	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

	device_info.queueCreateInfoCount = 1;

	device_info.pQueueCreateInfos = &queue_info;
	device_info.pEnabledFeatures  = &device_features;

	device_info.enabledExtensionCount   = sizeof(required_device_extensions) / sizeof(required_device_extensions[0]);
	device_info.ppEnabledExtensionNames = required_device_extensions;

	if (debug)
	{
		const char * device_layer_names[]
		{
			"VK_LAYER_LUNARG_standard_validation"
		};

		device_info.enabledLayerCount   = sizeof(device_layer_names) / sizeof(device_layer_names[0]);
		device_info.ppEnabledLayerNames = device_layer_names;
	}

	if (vkCreateDevice(state.physical_device, &device_info, nullptr, &state.device) != VK_SUCCESS)
	{
		return false;
	}

	vkGetDeviceQueue(state.device, state.graphics_queue_idx, 0, &state.graphics_queue);

	vkGetDeviceQueue(state.device, state.present_queue_idx, 0, &state.present_queue);

	//////
	// Command Pools
	//////

	VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

	pool_info.queueFamilyIndex = state.graphics_queue_idx;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(state.device, &pool_info, nullptr, &state.command_pool) != VK_SUCCESS)
	{
		return false;
	}

	VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = state.FRAMES_IN_FLIGHT;
	allocInfo.commandPool = state.command_pool;

	if (vkAllocateCommandBuffers(state.device, &allocInfo, state.command_bfr_list) != VK_SUCCESS)
	{
		return false;
	}

	//////
	// Synchronization
	//////

	VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint8_t i{0}; i < state.FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(state.device, &semaphore_info, nullptr, &state.image_available_sema[i]) != VK_SUCCESS)
		{
			return false;
		}

		if (vkCreateSemaphore(state.device, &semaphore_info, nullptr, &state.render_finished_sema[i]) != VK_SUCCESS)
		{
			return false;
		}

		if (vkCreateFence(state.device, &fence_info, nullptr, &state.fences[i]) != VK_SUCCESS)
		{
			return false;
		}
	}

	return true;
}
