#include "vk.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>

bool vk_swapchain_create()
{
	// Enumerate available surface formats

	uint32_t available_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(state.physical_device, state.surface, &available_format_count, nullptr);

	if (available_format_count == 0)
	{
		// Surface does not avail any supported pixel formats
		return false;
	}

	VkSurfaceFormatKHR available_formats[available_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(state.physical_device, state.surface, &available_format_count, available_formats);

	// Choose optimal surface format

	if (available_format_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		state.surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const auto & format : available_formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			state.surface_format = format;
		}
	}

	// Enumerate available present modes

	uint32_t available_present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(state.physical_device, state.surface, &available_present_mode_count, nullptr);

	if (available_present_mode_count == 0)
	{
		// Surface does not avail any supported present modes
		return false;
	}

	VkPresentModeKHR available_present_modes[available_present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(state.physical_device, state.surface, &available_present_mode_count, available_present_modes);

	// Choose optimal present mode

	VkPresentModeKHR best_present_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto & mode : available_present_modes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			best_present_mode = mode;
			break;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			best_present_mode = mode;
		}
	}

	state.present_mode = best_present_mode;

	// Enumerate available swap extents

	VkSurfaceCapabilitiesKHR surface_caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physical_device, state.surface, &surface_caps);

	if (surface_caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		state.swapchain_extent = surface_caps.currentExtent;
	}
	else
	{
		int32_t width;
		int32_t height;
		glfwGetFramebufferSize(state.window, &width, &height);

		VkExtent2D actual_extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

		actual_extent.width  = std::clamp(actual_extent.width, surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);

		state.swapchain_extent = actual_extent;
	}

	// Create swapchain

	state.backbuffer_size = surface_caps.minImageCount + 1;

	if (surface_caps.maxImageCount > 0 && state.backbuffer_size > surface_caps.maxImageCount)
	{
		state.backbuffer_size = surface_caps.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	createInfo.surface          = state.surface;
	createInfo.minImageCount    = state.backbuffer_size;
	createInfo.imageFormat      = state.surface_format.format;
	createInfo.imageColorSpace  = state.surface_format.colorSpace;
	createInfo.imageExtent      = state.swapchain_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform     = surface_caps.currentTransform;
	createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode      = state.present_mode;
	createInfo.clipped          = VK_TRUE;

	if (vkCreateSwapchainKHR(state.device, &createInfo, nullptr, &state.swapchain) != VK_SUCCESS)
	{
		return false;
	}

	// Retrieve swapchain images

	VkImage swapchain_images[state.backbuffer_size];

	vkGetSwapchainImagesKHR(state.device, state.swapchain, &state.backbuffer_size, swapchain_images);

	// Create image views

	for (uint32_t i{0}; i < state.backbuffer_size; ++i)
	{
		state.backbuffer_images[i] = swapchain_images[i];

		VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		view_info.image    = state.backbuffer_images[i];
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format   = state.surface_format.format;

		view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel   = 0;
		view_info.subresourceRange.levelCount     = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(state.device, &view_info, nullptr, &state.backbuffer_views[i]) != VK_SUCCESS)
		{
			return false;
		}
	}

	return true;
}

bool vk_swapchain_destroy()
{
	for (uint32_t i{0}; i < state.backbuffer_size; ++i)
	{
		vkDestroyImageView(state.device, state.backbuffer_views[i], nullptr);
	}

	vkDestroySwapchainKHR(state.device, state.swapchain, nullptr);

	return true;
}
