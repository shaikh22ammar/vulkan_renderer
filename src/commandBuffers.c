#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "rendererErrors.h"

#include "constants.h"
#include "utils/functionQueue.h"

extern VkDevice device;
extern uint32_t queueFamilyIndex;

extern VkCommandPool commandPool;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer *pCommandBuffers;
extern VkCommandBuffer transferCommandBuffer;


extern struct functionStack cleanupFunctions;




static RendererResult createCommandPool() {
	/* Command pools are allocators for command buffers.
	 * Since allocating and freeing command buffers individually would be
	 * too expensive, implementations offer specialized allocators called
	 * command pools that can reuse memory */

	VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
					/* Command buffers in this pool can be
					 * reset individually without having to 
					 * reset the entire pool */
	VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));
	return RENDERER_SUCCESS;
}


static RendererResult createTransferCommandPool() {
	/* The transfer command pool is specially used for transfer commands */


	VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
					/* Command buffers in this pool are short-lived
					 * */
	VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &transferCommandPool));
	return RENDERER_SUCCESS;
}

static RendererResult createCommandBuffers() {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	pCommandBuffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
	MALLOC_CHECK(pCommandBuffers);
	VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffers));

	commandBufferAllocateInfo.commandPool = transferCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &transferCommandBuffer));

	return RENDERER_SUCCESS;
}

static void destroyCommandPools() {
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyCommandPool(device, transferCommandPool, nullptr);
}

RendererResult initCommandBuffers() {
	RR_TRY(createCommandPool());
	RR_TRY(createTransferCommandPool());
	RR_TRY(createCommandBuffers());

	functionStack_insert(&cleanupFunctions, destroyCommandPools);
	return RENDERER_SUCCESS;
}

