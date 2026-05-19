#include <vulkan/vulkan_core.h>

extern VkDevice device;
extern VkSemaphore presentCompleteSemaphore;
extern VkSemaphore renderCompleteSemaphore;
extern VkFence drawFence;

void initSyncObjects() {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
	vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore);

	VkFenceCreateInfo fenceCreateInfo = {0};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(device, &fenceCreateInfo, nullptr, &drawFence);
}
