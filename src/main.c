#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef MAC_OS
#include <vulkan/vulkan_beta.h>
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
GLFWwindow *window = nullptr;

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

static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

static void cleanUp() {
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

extern VkResult initWindow();
extern VkResult initVulkan();
extern void cleanUp();
static void run() {
	if (initWindow() < VK_SUCCESS) goto cleanup;
	if (initVulkan() < VK_SUCCESS) goto cleanup;
	mainLoop();
cleanup:
	cleanUp();
}

int main() {
	run();
	return EXIT_SUCCESS;
}
