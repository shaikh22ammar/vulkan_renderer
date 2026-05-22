#include "rendererErrors.h"
#include <cglm/cglm.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
//#include "types.h"

extern VkDevice device;
extern VkPhysicalDevice physicalDevice;

struct Vertex {
	vec2 pos;
	vec3 color;
};
static struct Vertex *vertices;
extern int numVertices;
extern VkBuffer vertexBuffer;
extern VkDeviceMemory vertexBufferMemory;

static uint16_t *vertexIndices;
extern int numIndices;
extern VkBuffer indexBuffer;
extern VkDeviceMemory indexBufferMemory;

extern VkQueue queue;
extern VkCommandPool transferCommandPool;

extern VkResult createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usageFlag, 
		VkMemoryPropertyFlags memPropertyFlags,
		VkBuffer *pBuffer,
		VkDeviceMemory *pMemory);
extern VkResult copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

[[maybe_unused]] static void createTriangleVertices() {
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

	numIndices = 3;
	vertexIndices = malloc(numIndices * sizeof(uint16_t));
	if (!vertexIndices) handleRendererError(RENDERER_ERROR_OUT_OF_MEMORY, "createVertices", true); 
	vertexIndices[0] = 0;
	vertexIndices[1] = 1;
	vertexIndices[2] = 2;
}

[[maybe_unused]] static void createSquareVertices() {
	numVertices = 4;

	vertices = calloc(numVertices, sizeof(struct Vertex));
	if (!vertices) handleRendererError(RENDERER_ERROR_OUT_OF_MEMORY, "createVertices", true); 

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
	if (!vertexIndices) handleRendererError(RENDERER_ERROR_OUT_OF_MEMORY, "createVertices", true); 
	vertexIndices[0] = 0;
	vertexIndices[1] = 1;
	vertexIndices[2] = 2;
	vertexIndices[3] = 2;
	vertexIndices[4] = 3;
	vertexIndices[5] = 0;
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

static void createIndexBuffer() {
	VkDeviceSize indexBufferSize = numIndices * sizeof(uint32_t);
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		indexBufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&stagingBuffer, &stagingBufferMemory);
	void *data;
	VkResult result = vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
	handleVulkanError(result, "createIndexBuffer", true);
	memcpy(data, vertexIndices, indexBufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	result = createBuffer(
		indexBufferSize, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		&indexBuffer, &indexBufferMemory);
	handleVulkanError(result, "createBuffer", true);

	result = copyBuffer(stagingBuffer, indexBuffer, indexBufferSize);
	handleVulkanError(result, "copyBuffer", true);

	vkFreeMemory(device, stagingBufferMemory, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void initVertices() {
	createSquareVertices();
	createVertexBuffer();
	createIndexBuffer();
	free(vertices);
	vertices = NULL;
}
