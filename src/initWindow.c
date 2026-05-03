#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>

#include "rendererErrors.h"

extern const uint32_t WIDTH;
extern const uint32_t HEIGHT;
extern GLFWwindow *window;

void initWindow() {
	glfwInit();
	// prevent GLFW from loading openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Unable to create GLFW window\n");
		exit(RENDERER_ERROR_GLFW);
	}
}
