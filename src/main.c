#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "rendererErrors.h"

// glfw
uint32_t WIDTH = 800;
uint32_t HEIGHT = 600;
GLFWwindow *window = nullptr;

// init vulkan
VkInstance instance = nullptr;
VkSurfaceKHR surface = nullptr;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device = nullptr;
VkQueue queue = nullptr; 

VkSwapchainKHR swapChain = nullptr;
VkImage *swapChainImages = nullptr;
uint32_t swapChainImagesCount = 0;
VkSurfaceFormatKHR swapChainSurfaceFormat = {0};
VkExtent2D swapChainExtent = {0};
VkImageView *swapChainImageViews = nullptr;

// graphics pipeline
VkPipelineLayout pipelineLayout = nullptr;
VkPipeline graphicsPipeline = nullptr;

static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

static void cleanUp() {
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
extern void createGraphicsPipeline();
static void run() {
	initWindow();
	initVulkan();
	createGraphicsPipeline();
	mainLoop();
	cleanUp();
}

int main() {
	run();
	exit(RENDERER_SUCCESS);
}
