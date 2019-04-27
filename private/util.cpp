#include "vk.h"

#include <fstream>

uint32_t vk_util_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(state.physical_device, &mem_properties);

	for (unsigned int i = 0; i < mem_properties.memoryTypeCount; ++i)
	{
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return 0;
}

heap_allocation_t vk_util_file_data_read(const char * path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (file.is_open() == false)
	{
		return {};
	}

	size_t fileSize = (size_t) file.tellg();

	char * buffer = new char[fileSize];

	file.seekg(0);
	file.read(buffer, fileSize);

	file.close();

	return { buffer, fileSize };
}

void vk_util_file_data_destroy(const heap_allocation_t data)
{
	delete data.data;
}
