#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "rendererErrors.h"
#include "utils/functionQueue.h"

extern const uint32_t WIDTH;
extern const uint32_t HEIGHT;
extern GLFWwindow *window;
extern struct functionStack cleanupFunctions;

static void cleanUpGLFW() {
	glfwDestroyWindow(window);
	glfwTerminate();
#ifndef NDEBUG
	printf("Destroyed GLFW\n");
#endif
}
RendererResult initWindow() {
	glfwInit();
	// prevent GLFW from loading openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	if (!window) {
		RR_SET_ERROR(RENDERER_ERR_GLFW, 0, "GLFW window could not be created");
		return RENDERER_SUCCESS;
	}
	functionStack_insert(&cleanupFunctions, cleanUpGLFW);
	return RENDERER_SUCCESS;
}

