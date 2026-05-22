#include "rendererErrors.h"
#include <vulkan/vulkan_core.h>
#include "constants.h"
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

extern "C" {
	extern unsigned int currentFrameInFlight;

	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkExtent2D swapChainExtent;

	extern VkBuffer *pUniformBuffers;
	extern VkDeviceMemory *pUniformBuffersMemories;
	extern void **ppUniformBufferMemoryMapped;

	extern VkQueue queue;
	extern VkCommandPool transferCommandPool;

	extern void setBufferLabel(VkDevice device, VkBuffer buffer, const char* name);
	extern VkResult createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usageFlag, 
			VkMemoryPropertyFlags memPropertyFlags,
			VkBuffer *pBuffer,
			VkDeviceMemory *pMemory);
}

struct MVPmatrix {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

extern "C" const size_t sizeOfMVPmatrix = sizeof(struct MVPmatrix);

static void createUniformBuffers() {
	pUniformBuffers = (VkBuffer *) malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkBuffer));
	pUniformBuffersMemories = (VkDeviceMemory *) malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDeviceMemory));
	ppUniformBufferMemoryMapped = (void **) malloc(MAX_FRAMES_IN_FLIGHT * sizeof(void *));
	if (!pUniformBuffers || !pUniformBuffersMemories || !ppUniformBufferMemoryMapped)
		handleVulkanError(VK_ERROR_OUT_OF_HOST_MEMORY, "createUniformBuffers", true);

	VkMemoryPropertyFlags memPropertyFlags = 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDeviceSize bufferSize = sizeof(struct MVPmatrix);
		VkResult result = createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT, 
				memPropertyFlags, 
				pUniformBuffers + i, pUniformBuffersMemories + i);
		handleVulkanError(result, "createBuffer", true);
		result = vkMapMemory(device, pUniformBuffersMemories[i], 0, bufferSize, 0, ppUniformBufferMemoryMapped + i);
		handleVulkanError(result, "vkMapMemory", true);
	}
#ifndef NDEBUG
	setBufferLabel(device, pUniformBuffers[0], "UBO 0");
	setBufferLabel(device, pUniformBuffers[1], "UBO 1");
#endif
}

extern "C" void initUniformBuffers() {
	createUniformBuffers();
}

extern "C" void updateUniformBuffers() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto  currentTime = std::chrono::high_resolution_clock::now();
	float time        = std::chrono::duration<float>(currentTime - startTime).count();

	MVPmatrix ubo{};
	ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view  = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj =
	    glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
	ubo.proj[1][1] *= -1.0f;

	memcpy(ppUniformBufferMemoryMapped[currentFrameInFlight], &ubo, sizeof ubo);
}

extern "C" void destroyUniformBuffers() {
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkUnmapMemory(device, pUniformBuffersMemories[i]);
		vkFreeMemory(device, pUniformBuffersMemories[i], nullptr);
		vkDestroyBuffer(device, pUniformBuffers[i], nullptr);
	}
	free(pUniformBuffers); pUniformBuffers = nullptr;
	free(pUniformBuffersMemories); pUniformBuffersMemories = nullptr;
	free(ppUniformBufferMemoryMapped); ppUniformBufferMemoryMapped = nullptr;
}

