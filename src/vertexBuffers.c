#include "rendererErrors.h"
#include <cglm/cglm.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include "utils/alignment.h"
#include "utils/functionQueue.h"

/* Vertices and index data are packed in a single buffer */


extern VkDevice device;
extern VkPhysicalDevice physicalDevice;
extern VkQueue queue;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer transferCommandBuffer;




struct Vertex {
	vec2 pos;
	vec3 color;
};

static struct Vertex *vertices;
static uint16_t *vertexIndices;
const VkIndexType indexType = VK_INDEX_TYPE_UINT16;



extern VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
static constexpr int VERTEX_INPUT_ATTR_COUNT = 2;
static VkVertexInputBindingDescription vertexInputBindingDescription = {0};
static VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {0};
static VkVertexInputAttributeDescription pVertexInputAttributeDescriptions[VERTEX_INPUT_ATTR_COUNT];




extern VkDeviceSize indexOffset; 	// the offset from the beginning of vertex buffer where index data starts
extern VkDeviceSize vertexBufferSize; 	// total size of vertex buffer (vertex data + index data)
extern VkBuffer vertexBuffer;		// vertex buffer = [vertex data | padding | index data]
extern int numVertices;
extern int numIndices;

static VkDeviceMemory vertexBufferMemory;

static VkBuffer stagingBuffer;
static VkDeviceMemory stagingBufferMemory;

extern uint32_t findMemoryTypes(uint32_t supportedMemoryTypes, VkMemoryPropertyFlags requiredMemoryProperties);
#ifndef NDEBUG
extern void setBufferLabel(VkDevice device, VkBuffer buffer, const char* name);
#endif

[[maybe_unused]] static RendererResult createTriangleVertices() {
	numVertices = 3;

	vertices = calloc(numVertices, sizeof(struct Vertex));
	MALLOC_CHECK(vertices);

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

	numIndices = 3;
	vertexIndices = malloc(numIndices * sizeof(uint16_t));
	MALLOC_CHECK(vertexIndices);

	vertexIndices[0] = 0;
	vertexIndices[1] = 1;
	vertexIndices[2] = 2;

	return RENDERER_SUCCESS;
}

[[maybe_unused]] static RendererResult createSquareVertices() {
	numVertices = 4;

	vertices = calloc(numVertices, sizeof(struct Vertex));
	MALLOC_CHECK(vertices);

	vertices[0].pos[0] = -0.5f;
	vertices[0].pos[1] = -0.5f;
	vertices[0].color[0] = 1.0f;
	vertices[0].color[1] = 0.0f;
	vertices[0].color[2] = 0.0f;

	vertices[1].pos[0] = 0.5f;
	vertices[1].pos[1] = -0.5f;
	vertices[1].color[0] = 0.0f;
	vertices[1].color[1] = 1.0f;
	vertices[1].color[2] = 0.0f;

	vertices[2].pos[0] = 0.5f;
	vertices[2].pos[1] = 0.5f;
	vertices[2].color[0] = 0.0f;
	vertices[2].color[1] = 0.0f;
	vertices[2].color[2] = 1.0f;

	vertices[3].pos[0] = -0.5f;
	vertices[3].pos[1] = 0.5f;
	vertices[3].color[0] = 1.0f;
	vertices[3].color[1] = 1.0f;
	vertices[3].color[2] = 1.0f;

	numIndices = 6;
	vertexIndices = malloc(numIndices * sizeof(uint16_t));
	MALLOC_CHECK(vertexIndices);

	vertexIndices[0] = 0;
	vertexIndices[1] = 1;
	vertexIndices[2] = 2;
	vertexIndices[3] = 2;
	vertexIndices[4] = 3;
	vertexIndices[5] = 0;

	return RENDERER_SUCCESS;
}

static RendererResult updateVertexInputState() {
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.stride = sizeof(struct Vertex);
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	pVertexInputAttributeDescriptions[0].binding = 0;
	pVertexInputAttributeDescriptions[0].location = 0;
	pVertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	pVertexInputAttributeDescriptions[0].offset = offsetof(struct Vertex, pos);
	pVertexInputAttributeDescriptions[1].binding = 0;
	pVertexInputAttributeDescriptions[1].location = 1;
	pVertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	pVertexInputAttributeDescriptions[1].offset = offsetof(struct Vertex, color);

	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pNext = nullptr;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = pVertexInputAttributeDescriptions;

	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;

	return RENDERER_SUCCESS;
}


RendererResult createVertexBuffer() {
	/* staging buffer */
	VkDeviceSize vertexDataSize = sizeof vertices[0] * numVertices;
	VkDeviceSize indexDataSize = sizeof vertexIndices[0] * numIndices;

	/* index alignment is 2 if index type is of 16 bits otherwise 4 */
	VkDeviceSize indexAlignment = 2 + 2 * (indexType == VK_INDEX_TYPE_UINT32);

	/* indexoffset is the least multiple of indexAlignment no less than vertexDataSize */
	indexOffset = ALIGN_UP(vertexDataSize, indexAlignment);
	vertexBufferSize = indexOffset + indexDataSize;

	VkBufferCreateInfo stagingBuffCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.size = vertexBufferSize,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VK_CHECK(vkCreateBuffer(device, &stagingBuffCreateInfo, nullptr, &stagingBuffer));
	VkMemoryRequirements stagingBuffMemRequirements;
	vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBuffMemRequirements);
	VkMemoryAllocateInfo stagingBuffMemAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = stagingBuffMemRequirements.size,
		.memoryTypeIndex = 
			findMemoryTypes(
				stagingBuffMemRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				)
	};
	VK_CHECK(vkAllocateMemory(device, &stagingBuffMemAllocInfo, nullptr, &stagingBufferMemory));
	VK_CHECK(vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0));
	void *data;
	VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data));
	memcpy(data, vertices, vertexDataSize);
	memcpy((char *)data + indexOffset, vertexIndices, indexDataSize);
	vkUnmapMemory(device, stagingBufferMemory);

	/* device local buffer */
	VkBufferCreateInfo vertextBuffCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.size = vertexBufferSize
	};
	VK_CHECK(vkCreateBuffer(device, &vertextBuffCreateInfo, nullptr, &vertexBuffer));
	VkMemoryRequirements vertexBuffMemRequirements;
	vkGetBufferMemoryRequirements(device, vertexBuffer, &vertexBuffMemRequirements);
	VkMemoryAllocateInfo vertextBuffMemAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = vertexBuffMemRequirements.size,
		.memoryTypeIndex =
			findMemoryTypes(vertexBuffMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_CHECK(vkAllocateMemory(device, &vertextBuffMemAllocInfo, nullptr, &vertexBufferMemory));
	VK_CHECK(vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0));

	/* copying staging buffer to vertex buffer */
	VkCommandBufferBeginInfo transferBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	VK_CHECK(vkBeginCommandBuffer(transferCommandBuffer, &transferBufferBeginInfo));
	VkBufferCopy stageToVertCopyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = vertexBufferSize
	};
	vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, vertexBuffer, 1, &stageToVertCopyRegion);
	VK_CHECK(vkEndCommandBuffer(transferCommandBuffer));

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
	VK_CHECK(vkQueueSubmit2(queue, 1, &transferSubmitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(queue));

	vkResetCommandPool(device, transferCommandPool, 0);

	return RENDERER_SUCCESS;
}

static void destroyBufferAndMemories() {
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

extern struct functionStack cleanupFunctions;
RendererResult initVertices() {
	constexpr int numFunctions = 3;

	RendererResult (*functionsToCall[numFunctions])(void) = {
		createSquareVertices,
		updateVertexInputState,
		createVertexBuffer
	};
	for (int i = 0; i < numFunctions; i++) {
		RR_TRY(functionsToCall[i]());
	}

	functionStack_insert(&cleanupFunctions, destroyBufferAndMemories);
	free(vertices);
	vertices = NULL;

#ifndef NDEBUG
	setBufferLabel(device, vertexBuffer, "Vertex buffer");
#endif

	return RENDERER_SUCCESS;
}
