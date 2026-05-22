#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "rendererErrors.h"

#include "constants.h"

extern VkDevice device;
extern uint32_t queueFamilyIndex;
extern VkCommandPool commandPool;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer *pCommandBuffers;
extern VkImage *swapChainImages;
extern VkImageView *swapChainImageViews;
extern VkExtent2D swapChainExtent;
extern VkPipeline graphicsPipeline;
extern VkBuffer vertexBuffer;
extern VkBuffer indexBuffer;
extern int numIndices;
extern int numVertices;

extern unsigned int currentFrameInFlight;

static VkResult createCommandPool() {
	/* Command pools are allocators for command buffers.
	 * Since allocating and freeing command buffers individually would be
	 * too expensive, implementations offer specialized allocators called
	 * command pools that can reuse memory */
	VkResult result;
	VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
					/* Command buffers in this pool can be
					 * reset individually withou having to 
					 * reset the entire pool */
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
	return result;
}


static VkResult createTransferCommandPool() {
	/* Command pools are allocators for command buffers.
	 * Since allocating and freeing command buffers individually would be
	 * too expensive, implementations offer specialized allocators called
	 * command pools that can reuse memory */
	VkResult result;
	VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
					/* Command buffers in this pool are short-lived
					 * */
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &transferCommandPool);
	return result;
}

static VkResult createCommandBuffer() {
	VkResult result;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	pCommandBuffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
	if (!pCommandBuffers) return VK_ERROR_OUT_OF_HOST_MEMORY;
	result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffers);

	return result;
}


void initCommandBuffers() {
	constexpr int numFunctions = 3;
	VkResult (*functionsToCall[numFunctions])(void) = {
		createCommandPool,
		createTransferCommandPool,
		createCommandBuffer
	};
	const char *functionsToCallNames[numFunctions] = {
		"createCommandPool",
		"createTransferCommandPool",
		"createCommandBuffer"
	};
	for (int i = 0; i < numFunctions; i++) {
		handleVulkanError(functionsToCall[i](), functionsToCallNames[i], true);
	}
}
