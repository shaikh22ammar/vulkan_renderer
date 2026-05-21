#include "rendererErrors.h"
#include <cglm/cglm.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
//#include "types.h"

struct Vertex {
	vec2 pos;
	vec3 color;
};
static struct Vertex *vertices;

extern VkCommandPool transferCommandPool;
extern VkQueue queue;
extern VkPhysicalDevice physicalDevice;
extern int numVertices;
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
	numVertices = 3;

	vertices = calloc(numVertices, sizeof(struct Vertex));
	if (!vertices) handleRendererError(RENDERER_ERROR_OUT_OF_MEMORY, "createVertices", true); 

	vertices[0].pos[0] = 0.0f;
	vertices[0].pos[1] = -0.5f;
	vertices[0].color[0] = 1.0f;
	vertices[0].color[1] = 0.0f;
	vertices[0].color[2] = 0.0f;

	vertices[1].pos[0] = 0.5f;
	vertices[1].pos[1] = 0.5f;
	vertices[1].color[0] = 0.0f;
	vertices[1].color[1] = 1.0f;
	vertices[1].color[2] = 0.0f;

	vertices[2].pos[0] = -0.5f;
	vertices[2].pos[1] = 0.5f;
	vertices[2].color[0] = 0.0f;
	vertices[2].color[1] = 0.0f;
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

static VkResult copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
	VkCommandBufferAllocateInfo transferBufferAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = transferCommandPool,
		.level = 0,
		.commandBufferCount = 1
	};
	VkCommandBuffer transferCommandBuffer;
	VkResult result = vkAllocateCommandBuffers(device, &transferBufferAllocateInfo, &transferCommandBuffer);
	if (result < VK_SUCCESS) return result;

	VkCommandBufferBeginInfo transferCommandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	result = vkBeginCommandBuffer(transferCommandBuffer, &transferCommandBufferBeginInfo);
	if (result < VK_SUCCESS) return result;

	VkBufferCopy bufferCopy = {.srcOffset = 0, .dstOffset = 0, .size = size};
	vkCmdCopyBuffer(transferCommandBuffer, src, dst, 1, &bufferCopy);
	 
	result = vkEndCommandBuffer(transferCommandBuffer);
	if (result < VK_SUCCESS) return result;

	VkCommandBufferSubmitInfo transferCommandBufferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = transferCommandBuffer,
		.deviceMask = 0
	};

	VkSubmitInfo2 transferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &transferCommandBufferSubmitInfo
	};
	result = vkQueueSubmit2(queue, 1, &transferSubmitInfo, VK_NULL_HANDLE);
	if (result < VK_SUCCESS) return result;
	result = vkQueueWaitIdle(queue);
	if (result < VK_SUCCESS) return result;

	result = vkResetCommandPool(device, transferCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	return result;
}

static void createVertexBuffer() {
	VkDeviceSize vertexBufferSize = numVertices * sizeof vertices[0];
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		vertexBufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&stagingBuffer, &stagingBufferMemory);
	void *data;
	VkResult result = vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
	handleVulkanError(result, "createVertexBuffer", true);
	memcpy(data, vertices, vertexBufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	result = createBuffer(
		vertexBufferSize, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		&vertexBuffer, &vertexBufferMemory);
	handleVulkanError(result, "createBuffer", true);

	result = copyBuffer(stagingBuffer, vertexBuffer, vertexBufferSize);
	handleVulkanError(result, "copyBuffer", true);

	vkFreeMemory(device, stagingBufferMemory, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void initVertices() {
	createVertices();
	createVertexBuffer();
	free(vertices);
	vertices = NULL;
}
