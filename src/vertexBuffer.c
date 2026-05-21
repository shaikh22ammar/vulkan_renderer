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

static uint32_t findMemoryType(uint32_t supportedMemoryTypes, VkMemoryPropertyFlags requiredMemoryProperties) {
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

static VkResult createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usageFlag, 
		VkMemoryPropertyFlags memPropertyFlags,
		VkBuffer *pBuffer,
		VkDeviceMemory *pMemory) {

	VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = size,
		.usage = usageFlag,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, pBuffer);
	if (result < VK_SUCCESS) return result;
	if (!pMemory) return VK_SUCCESS;

	VkMemoryRequirements bufferMemoryRequirements;
	vkGetBufferMemoryRequirements(device, *pBuffer, &bufferMemoryRequirements);
	uint32_t i = findMemoryType(
			bufferMemoryRequirements.memoryTypeBits, 
			memPropertyFlags);
	if (i == VK_MAX_MEMORY_TYPES) 
		return VK_ERROR_UNKNOWN;

	VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = bufferMemoryRequirements.size,
		.memoryTypeIndex = i
	};

	result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, pMemory);
	if (result < VK_SUCCESS) return result;

	result = vkBindBufferMemory(device, *pBuffer, *pMemory, 0);
	if (result < VK_SUCCESS) return result;

	return VK_SUCCESS;
}

static void createVertexBuffer() {
	VkDeviceSize vertexBufferSize = numVertices * sizeof vertices[0];
	createBuffer(
		vertexBufferSize, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&vertexBuffer, &vertexBufferMemory);
	void *data;
	VkResult result = vkMapMemory(device, vertexBufferMemory, 0, vertexBufferSize, 0, &data);
	handleVulkanError(result, "createVertexBuffer", true);
	memcpy(data, vertices, vertexBufferSize);
	vkUnmapMemory(device, vertexBufferMemory);
}

void initVertices() {
	createVertices();
	createVertexBuffer();
}
