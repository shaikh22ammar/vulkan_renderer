#include <cglm/cglm.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

extern VkPhysicalDevice physicalDevice;

uint32_t findMemoryTypes(uint32_t supportedMemoryTypes, VkMemoryPropertyFlags requiredMemoryProperties) {
	VkPhysicalDeviceMemoryProperties2 deviceMemoryProperies = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
		.pNext = nullptr
	};
	vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &deviceMemoryProperies);

	for (uint32_t i = 0; i < deviceMemoryProperies.memoryProperties.memoryTypeCount; i++) {
		if ((supportedMemoryTypes & (1 << i))
		&& (deviceMemoryProperies.memoryProperties.memoryTypes[i].propertyFlags & requiredMemoryProperties))
			return i;
	}
	return VK_MAX_MEMORY_TYPES;
}

#ifndef NDEBUG
extern VkInstance instance;
void setBufferLabel(VkDevice device, VkBuffer buffer, const char* name) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = {0};
	nameInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType  = VK_OBJECT_TYPE_BUFFER;
	nameInfo.objectHandle = (uint64_t)buffer;
	nameInfo.pObjectName = name;
	PFN_vkSetDebugUtilsObjectNameEXT func = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	func(device, &nameInfo);
}
#endif
