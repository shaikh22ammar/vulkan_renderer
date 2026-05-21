#include "rendererErrors.h"
#include <cglm/cglm.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include "types.h"

extern VkPhysicalDevice physicalDevice;
extern const int numVertices;
extern struct Vertex *vertices;
extern VkDevice device;
extern VkBuffer vertexBuffer;
extern VkDeviceMemory vertexBufferMemory;

extern VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;

static uint32_t findMemoryType(uint32_t supportedMemoryTypes, VkMemoryPropertyFlagBits requiredMemoryProperties) {
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

static void createVertices() {
	vertices = calloc(numVertices, sizeof(struct Vertex));
	if (!vertices) handleRendererError(RENDERER_ERROR_OUT_OF_MEMORY, "createVertices", true); 

	vertices[0].pos[0] = 0.0f;
	vertices[0].pos[1] = -0.5f;
	vertices[0].color[0] = 1.0f;
	vertices[0].color[1] = 1.0f;
	vertices[0].color[2] = 1.0f;

	vertices[1].pos[0] = 0.5f;
	vertices[1].pos[1] = 0.5f;
	vertices[1].color[0] = 1.0f;
	vertices[1].color[1] = 1.0f;
	vertices[1].color[2] = 1.0f;

	vertices[2].pos[0] = -0.5f;
	vertices[2].pos[1] = 0.5f;
	vertices[2].color[2] = 1.0f;
	vertices[2].color[0] = 1.0f;
	vertices[2].color[1] = 1.0f;
	vertices[2].color[2] = 1.0f;
}

static void createVertexBuffer() {
	VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = 3 * sizeof vertices[0],
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &vertexBuffer);
	handleVulkanError(result, "vkCreateBuffer", true);
	VkMemoryRequirements bufferMemoryRequirements;
	vkGetBufferMemoryRequirements(device, vertexBuffer, &bufferMemoryRequirements);
	uint32_t i = findMemoryType(bufferMemoryRequirements.memoryTypeBits, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (i == VK_MAX_MEMORY_TYPES) 
		handleRendererError(RENDERER_ERROR_INCOMPATIBILITY, "findMemoryType", true);

	VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = bufferMemoryRequirements.size,
		.memoryTypeIndex = i
	};

	result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &vertexBufferMemory);
	handleVulkanError(result, "vkAllocateMemory", true);

	result = vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
	handleVulkanError(result, "vkBindBufferMemory", true);

	void *data;
	result = vkMapMemory(device, vertexBufferMemory, 0, bufferMemoryRequirements.size, 0, &data);
	memcpy(data, vertices, bufferCreateInfo.size);
	vkUnmapMemory(device, vertexBufferMemory);
}

void initVertices() {
	createVertices();
	createVertexBuffer();
}
