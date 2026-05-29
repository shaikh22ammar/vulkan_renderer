#include "rendererErrors.h"
#include <cglm/cglm.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include "utils/alignment.h"
#include "utils/functionQueue.h"
#include "types.h"

/* Vertices and index data are packed in a single buffer */


extern VkDevice device;
extern VkPhysicalDevice physicalDevice;
extern VkQueue queue;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer transferCommandBuffer;



static struct Vertex *vertices;
static uint32_t *vertexIndices;
const VkIndexType indexType = VK_INDEX_TYPE_UINT32;



extern VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;




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

static RendererResult createSquareVertices() {
	numVertices = 8;

	vertices = calloc(numVertices, sizeof(struct Vertex));
	MALLOC_CHECK(vertices);

	vertices[0].pos[0] = -0.5f;
	vertices[0].pos[1] = -0.5f;

	vertices[1].pos[0] = 0.5f;
	vertices[1].pos[1] = -0.5f;
	vertices[1].uv[0] = 1;
	vertices[1].uv[1] = 0;

	vertices[2].pos[0] = 0.5f;
	vertices[2].pos[1] = 0.5f;
	vertices[2].uv[0] = 1;
	vertices[2].uv[1] = 1;

	vertices[3].pos[0] = -0.5f;
	vertices[3].pos[1] = 0.5f;
	vertices[3].uv[0] = 0;
	vertices[3].uv[1] = 1;

	for(int i = 0; i < 4; i++) {
		vertices[i+4].pos[0] = vertices[i].pos[0];
		vertices[i+4].pos[1] = vertices[i].pos[1];
		vertices[i+4].pos[2] = 0.5f;
		
		vertices[i+4].uv[0] = vertices[i].uv[0];
		vertices[i+4].uv[1] = vertices[i].uv[1];
	}

	numIndices = 12;
	vertexIndices = malloc(numIndices * sizeof(uint16_t));
	MALLOC_CHECK(vertexIndices);

	for (int j = 0; j <= 1; j++) {
		vertexIndices[0+6*j] = 0+4*j;
		vertexIndices[1+6*j] = 1+4*j;
		vertexIndices[2+6*j] = 2+4*j;
		vertexIndices[3+6*j] = 2+4*j;
		vertexIndices[4+6*j] = 3+4*j;
		vertexIndices[5+6*j] = 0+4*j;
	}

	return RENDERER_SUCCESS;
}

static RendererResult updateVertexInputState() {
	static constexpr int VERTEX_INPUT_ATTR_COUNT = 2;
	static VkVertexInputBindingDescription vertexInputBindingDescription = {0};
	static VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {0};
	static VkVertexInputAttributeDescription pVertexInputAttributeDescriptions[VERTEX_INPUT_ATTR_COUNT];

	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.stride = sizeof(struct Vertex);
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	pVertexInputAttributeDescriptions[0].binding = 0;
	pVertexInputAttributeDescriptions[0].location = 0;
	pVertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	pVertexInputAttributeDescriptions[0].offset = offsetof(struct Vertex, pos);
	pVertexInputAttributeDescriptions[1].binding = 0;
	pVertexInputAttributeDescriptions[1].location = 1;
	pVertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	pVertexInputAttributeDescriptions[1].offset = offsetof(struct Vertex, uv);

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

#ifndef NDEBUG
extern RendererResult rrSetDebugObjectName (
		VkObjectType objectType,
		uint64_t objectHandle,
		const char *objectName
		);
#endif

void loadModel(struct Vertex **vertices, uint32_t **indices, int *numVertices, int *numIndices);
extern struct functionStack cleanupFunctions;
RendererResult initVertices() {
	//RR_TRY(createSquareVertices());
	loadModel(&vertices, &vertexIndices, &numVertices, &numIndices);
	RR_TRY(updateVertexInputState());
	RR_TRY(createVertexBuffer());

	functionStack_insert(&cleanupFunctions, destroyBufferAndMemories);
	//free(vertices);
	//free(vertexIndices);
	vertices = NULL;

#ifndef NDEBUG
	rrSetDebugObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t) vertexBuffer, "vertex buffer");
#endif

	return RENDERER_SUCCESS;
}
