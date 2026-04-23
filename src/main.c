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

static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

static void cleanUp() {
	if (device) vkDestroyDevice(device, nullptr);
	if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
	if (instance) vkDestroyInstance(instance, nullptr);
	if (window) glfwDestroyWindow(window);
	glfwTerminate();
}

extern VkResult initWindow();
extern VkResult initVulkan();
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
