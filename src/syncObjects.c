#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "constants.h"
#include "rendererErrors.h"
#include "utils/functionQueue.h"

extern uint32_t swapChainImagesCount;
extern VkDevice device;


extern VkSemaphore *pAcquiredImageSemaphores;
/* This semaphore is signaled when a swapchain image is acquired.
 * There are as many of these as frames in flight.
 * It is unsignaled through draw commands execution. 
 * It exists to ensure that before a draw command begins execution,
 * the swapchain image is actually acquired. */
extern VkFence *pDrawingDoneFences;
/* This fence is signaled when the draw commands of a FIF finish execution. 
 * There are as many of these as frames in flight.
 * It exists to ensure that the CPU waits before 
 * reseting and re-recording the buffer of the draw command
 * of that particular FIF*/
extern VkSemaphore *pRenderingDoneSemaphores;
/* This semaphore is signaled when the draw commands of a FIF finish execution.
 * There are as many of these as swapchain images.
 * The draw signals the semaphore corresponding to the swapchain image they are 
 * attached to. It is unsignaled by presentation operation.
 * Note that if this was indexed according to FIF then when singaling it 
 * through the draw commands, it is possible that it's still in use (signaled)
 * by the previous presentation operation. 
 * Remember that draw commands wait on an image being available and thus implicitly
 * also wait for this semaphore of that image to be unsignaled. */

static RendererResult createSyncObjects() {
	pAcquiredImageSemaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	pRenderingDoneSemaphores = malloc(sizeof(VkSemaphore) * swapChainImagesCount);
	pDrawingDoneFences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT); 
	MALLOC_CHECK(pAcquiredImageSemaphores && pRenderingDoneSemaphores && pDrawingDoneFences);


	VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	VkFenceCreateInfo fenceCreateInfo = {0};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { 
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, pAcquiredImageSemaphores + i)); 
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, pDrawingDoneFences + i));
	}

	for (unsigned int i = 0; i < swapChainImagesCount; i++) {
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, pRenderingDoneSemaphores + i));
	}

	return RENDERER_SUCCESS;
}

static void destroySyncObjects() {
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

extern struct functionStack cleanupFunctions;
RendererResult initSyncObjects() {
	RR_TRY(createSyncObjects());
	functionStack_insert(&cleanupFunctions, destroySyncObjects);
	return RENDERER_SUCCESS;
}
