#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "rendererErrors.h"

// glfw
uint32_t WIDTH = 800;
uint32_t HEIGHT = 600;
GLFWwindow *window = nullptr;

// init vulkan
VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue queue = VK_NULL_HANDLE; 
uint32_t queueFamilyIndex = 0;
VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;


VkSwapchainKHR swapChain = VK_NULL_HANDLE;
VkImage *swapChainImages = VK_NULL_HANDLE;
uint32_t swapChainImagesCount = 0;
VkSurfaceFormatKHR swapChainSurfaceFormat = {0};
VkExtent2D swapChainExtent = {0};
VkImageView *swapChainImageViews = VK_NULL_HANDLE;

// graphics pipeline
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
VkPipeline graphicsPipeline = VK_NULL_HANDLE;

// command buffers
VkCommandPool commandPool = VK_NULL_HANDLE;
VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

// semaphores
VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
VkFence drawFence = VK_NULL_HANDLE;


void drawFrame();
static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
}

static void cleanUp() {
	// sync objects
	vkDestroyFence(device, drawFence, nullptr);
	vkDestroySemaphore(device, renderCompleteSemaphore, nullptr);
	vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

	// command pool
	vkDestroyCommandPool(device, commandPool, nullptr);

	// graphics pipeline
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	// swapchain image views
	for (uint32_t i = 0; i < swapChainImagesCount; i++) {
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	swapChainImageViews = nullptr;

	// swapchain
	vkDestroySwapchainKHR(device, swapChain, nullptr);
	swapChainImages = nullptr;

	// device
	vkDestroyDevice(device, nullptr);
	device = nullptr;

	// surface
	vkDestroySurfaceKHR(instance, surface, nullptr);
	surface = nullptr;

	// instance
	vkDestroyInstance(instance, nullptr);
	instance = nullptr;

	// glfw
	glfwDestroyWindow(window);
	window = nullptr;
	glfwTerminate();
}

extern void initWindow();
extern void initVulkan();
extern void initGraphicsPipeline();
extern void initCommandBuffers();
extern void initSyncObjects();
static void run() {
	initWindow();
	initVulkan();
	initGraphicsPipeline();
	initCommandBuffers();
	initSyncObjects();
	mainLoop();
	cleanUp();
}

int main() {
	run();
	exit(RENDERER_SUCCESS);
}

extern void recordCommandBuffer(uint32_t);
void drawFrame() {
	VkResult result;
	result = vkWaitForFences(device, 1, &drawFence, VK_TRUE, UINT64_MAX);
	handleVulkanError(result, "vkWaitForFences", true);
	result = vkResetFences(device, 1, &drawFence);
	handleVulkanError(result, "vkResetFences", true);
	uint32_t nextImageIndex;
	result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &nextImageIndex);
	handleVulkanError(result, "vkAcquireNextImageKHR", true);
	recordCommandBuffer(nextImageIndex);

	VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = commandBuffer,
		.deviceMask = 0
	};

	VkSemaphoreSubmitInfo waitForPresentCompleteSemaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = presentCompleteSemaphore,
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.deviceIndex = 0
	};

	VkSemaphoreSubmitInfo signalRenderCompleteSemaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = renderCompleteSemaphore,
		.stageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		.deviceIndex = 0
	};

	VkSubmitInfo2 submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &waitForPresentCompleteSemaphoreInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commandBufferSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signalRenderCompleteSemaphoreInfo
	};

	result = vkQueueSubmit2(queue, 1, &submitInfo, drawFence);
	handleVulkanError(result, "vkQueueSubmit2", true);

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderCompleteSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapChain,
		.pImageIndices = &nextImageIndex,
		.pResults = nullptr
	};

	result = vkQueuePresentKHR(queue, &presentInfo);
	handleVulkanError(result, "vkQueuePresentKHR", true);
}
