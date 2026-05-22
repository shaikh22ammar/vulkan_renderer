#include "constants.h"
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
extern VkDescriptorPool descriptorPool;
extern VkDescriptorSet *pDescriptorSets;
extern VkDevice device;
extern VkDescriptorSetLayout descriptorSetLayout;

extern VkBuffer *pUniformBuffers;
extern VkDeviceMemory *pUniformBuffersMemories;
extern void **ppUniformBufferMemoryMapped;

static VkResult createDescriptorPools() {
	VkDescriptorPoolSize descriptorPoolSize = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
		.descriptorCount = MAX_FRAMES_IN_FLIGHT
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = MAX_FRAMES_IN_FLIGHT,
		.poolSizeCount = 1,
		.pPoolSizes = & descriptorPoolSize
	};

	VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
	if (result < VK_SUCCESS) return result;	

	return result;
}

static VkResult createDescriptorSets() {
	pDescriptorSets = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSet));
	if (!pDescriptorSets) return VK_ERROR_OUT_OF_HOST_MEMORY;
	VkDescriptorSetLayout pDescriptorSetsLayouts[MAX_FRAMES_IN_FLIGHT] = {
		descriptorSetLayout, descriptorSetLayout
	};
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
		.pSetLayouts = pDescriptorSetsLayouts
	};
	VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, pDescriptorSets);
	return result;
}

static VkResult populateDescriptorSets() {
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo descriptorBufferInfo = {
			.buffer = uniform
		}
	}
}

void initDescriptors() {
	constexpr unsigned int numFunctions = 2;
	VkResult (*functionsToCall[numFunctions])(void) = {
		createDescriptorPools,
		createDescriptorSets
	};
	const char *functionNames[numFunctions] = {
		"createDescriptorPools",
		"createDescriptorSets"
	};
	for (unsigned int i = 0; i < numFunctions; i++) {
		handleVulkanError(functionsToCall[i](), functionNames[i], true);
	}
}

void destroyDescriptors() {
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	free(pDescriptorSets);
	pDescriptorSets = nullptr;
}
