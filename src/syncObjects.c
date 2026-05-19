#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "constants.h"
#include "rendererErrors.h"

extern uint32_t swapChainImagesCount;
extern VkDevice device;
extern VkSemaphore *pAcquiredImageSemaphores;
extern VkFence *pDrawingDoneFences;
extern VkSemaphore *pRenderingDoneSemaphores;

static VkResult createSyncObjects() {
	pAcquiredImageSemaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	pRenderingDoneSemaphores = malloc(sizeof(VkSemaphore) * swapChainImagesCount);
	pDrawingDoneFences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT); 

	if (!pAcquiredImageSemaphores || !pRenderingDoneSemaphores || !pRenderingDoneSemaphores) 
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	VkFenceCreateInfo fenceCreateInfo = {0};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { 
		result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, pAcquiredImageSemaphores + i); 
		if (result < VK_SUCCESS) return result; 
		result = vkCreateFence(device, &fenceCreateInfo, nullptr, pDrawingDoneFences + i);
		if (result < VK_SUCCESS) return result;
	}

	for (unsigned int i = 0; i < swapChainImagesCount; i++) {
		result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, pRenderingDoneSemaphores + i);
		if (result < VK_SUCCESS) return result;
	}

	return VK_SUCCESS;
}

void destroySyncObjects() {
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { 
		vkDestroyFence(device, pDrawingDoneFences[i], nullptr);
		vkDestroySemaphore(device, pAcquiredImageSemaphores[i], nullptr);
	}

	for (unsigned int i = 0; i < swapChainImagesCount; i++) {
		vkDestroySemaphore(device, pRenderingDoneSemaphores[i], nullptr);
	}

	free(pDrawingDoneFences);
	pDrawingDoneFences = nullptr;
	free(pAcquiredImageSemaphores);
	pAcquiredImageSemaphores = nullptr;
	free(pRenderingDoneSemaphores);
	pRenderingDoneSemaphores = nullptr;
}

void initSyncObjects() {
	handleVulkanError(createSyncObjects(), "createSyncObjects", true);
}
