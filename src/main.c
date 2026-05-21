#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <cglm/cglm.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "rendererErrors.h"
#include "constants.h"
#include "types.h"


// glfw
/* Submit the drawing commands */
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

// swapchain
VkSwapchainKHR swapChain = VK_NULL_HANDLE;
VkImage *swapChainImages = VK_NULL_HANDLE;
uint32_t swapChainImagesCount = 0;
VkSurfaceFormatKHR swapChainSurfaceFormat = {0};
VkExtent2D swapChainExtent = {0};
VkImageView *swapChainImageViews = VK_NULL_HANDLE;

// graphics pipeline
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
VkPipeline graphicsPipeline = VK_NULL_HANDLE;

// command pool
VkCommandPool commandPool = VK_NULL_HANDLE;


// frame in flight
unsigned int currentFrameInFlight = 0;

VkCommandBuffer *pCommandBuffers = nullptr;
/* One command buffer for every frame in flight (FIF) */

VkSemaphore *pAcquiredImageSemaphores = nullptr;
/* This semaphore is signaled when a swapchain image is acquired.
 * There are as many of these as frames in flight.
 * It is unsignaled through draw commands execution. 
 * It exists to ensure that before a draw command begins execution,
 * the swapchain image is actually acquired. */
VkFence *pDrawingDoneFences = nullptr;
/* This fence is signaled when the draw commands of a FIF finish execution. 
 * There are as many of these as frames in flight.
 * It exists to ensure that the CPU waits before 
 * reseting and re-recording the buffer of the draw command
 * of that particular FIF*/
VkSemaphore *pRenderingDoneSemaphores = nullptr;
/* This semaphore is signaled when the draw commands of a FIF finish execution.
 * There are as many of these as swapchain images.
 * The draw signals the semaphore corresponding to the swapchain image they are 
 * attached to. It is unsignaled by presentation operation.
 * Note that if this was indexed according to FIF then when singaling it 
 * through the draw commands, it is possible that it's still in use (signaled)
 * by the previous presentation operation. 
 * Remember that draw commands wait on an image being available and thus implicitly
 * also wait for this semaphore of that image to be unsignaled. */

struct Vertex *vertices = nullptr;
const int numVertices = 3;
VkBuffer vertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

void drawFrame();
static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(device);
}
extern void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
extern void destroySyncObjects();
static void cleanUp() {
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	free(vertices);
	vertices = NULL;

	// sync objects
	destroySyncObjects();

	// command pool
	vkDestroyCommandPool(device, commandPool, nullptr);
	free(pCommandBuffers);
	pCommandBuffers = nullptr;

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
	destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
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
extern void initVertices();
static void run() {
	initWindow();
	initVulkan();
	initGraphicsPipeline();
	initCommandBuffers();
	initSyncObjects();
	initVertices();
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

	/* Before recording into the command buffer 
	 * of the current FIF, make sure it has been executed */
	result = vkWaitForFences(device, 1, pDrawingDoneFences + currentFrameInFlight, VK_TRUE, UINT64_MAX);
	handleVulkanError(result, "vkWaitForFences", true);
	result = vkResetFences(device, 1, pDrawingDoneFences + currentFrameInFlight);
	handleVulkanError(result, "vkResetFences", true);

	/* Acquire the next image in the swapchain to be presented.
	 * Signal when image is acquired */
	uint32_t nextImageIndex;
	result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, pAcquiredImageSemaphores[currentFrameInFlight], VK_NULL_HANDLE, &nextImageIndex);
	handleVulkanError(result, "vkAcquireNextImageKHR", true);


	/* Start recording the command buffer for the current FIF 
	 * with the next image in swapchain as attachment */
	recordCommandBuffer(nextImageIndex);

	VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = pCommandBuffers[currentFrameInFlight],
		.deviceMask = 0
	};

	/* Do not start executing the command buffer commands
	 * for color attachment until the next image
	 * has been acquired */
	VkSemaphoreSubmitInfo waitForImageAcquisitionSemaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = pAcquiredImageSemaphores[currentFrameInFlight],
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.deviceIndex = 0
	};

	/* Signal the render complete semaphore for the current image
	 * after all graphics commands in the buffer (and all the commands 
	 * submitted prior to the queue) is done */
	VkSemaphoreSubmitInfo signalRenderCompleteSemaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = pRenderingDoneSemaphores[nextImageIndex],
		.stageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		.deviceIndex = 0
	};

	VkSubmitInfo2 submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &waitForImageAcquisitionSemaphoreInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commandBufferSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signalRenderCompleteSemaphoreInfo
	};

	/* Submit the drawing commands.
	 * Signal the drawFence when the commands finish executing */
	result = vkQueueSubmit2(queue, 1, &submitInfo, pDrawingDoneFences[currentFrameInFlight]);
	handleVulkanError(result, "vkQueueSubmit2", true);

	/* Wait for the rendering to be done for the current FIF, 
	 * to the next swapchain image. Present the render to the next image */
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = pRenderingDoneSemaphores + nextImageIndex,
		.swapchainCount = 1,
		.pSwapchains = &swapChain,
		.pImageIndices = &nextImageIndex,
		.pResults = nullptr
	};

	result = vkQueuePresentKHR(queue, &presentInfo);
	handleVulkanError(result, "vkQueuePresentKHR", true);

	currentFrameInFlight = (currentFrameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;
}
