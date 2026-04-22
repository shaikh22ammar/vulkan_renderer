#include <stdio.h>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdint.h>

extern const uint32_t WIDTH;
extern const uint32_t HEIGHT;
extern GLFWwindow *window;

VkResult initWindow() {
	glfwInit();
	// prevent GLFW from loading openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Unable to create GLFW window\n");
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	return VK_SUCCESS;
}
