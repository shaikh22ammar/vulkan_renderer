#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
static GLFWwindow *window;

static void initWindow() {
	glfwInit();
	// prevent GLFW from loading openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

static VkInstance instance;
static VkResult createInstance() {
	VkResult result = VK_SUCCESS;

	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


	// Required for MoltenVK
	#ifdef MAC_OS
	uint32_t extensionCount = glfwExtensionCount + 1;
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	const char **requiredExtensions = malloc((sizeof *glfwExtensions) * extensionCount);
	if (requiredExtensions == nullptr) {
		fprintf(stderr, "Failed allocating memory for extensions\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto failure;
	}
	for (uint32_t i = 0; i < extensionCount - 1; i++) {
		requiredExtensions[i] = glfwExtensions[i];
	}
	requiredExtensions[extensionCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	#else
	uint32_t extensionCount = glfwExtensionCount;
	const char **requiredExtensions = malloc((sizeof *glfwExtensions) * extensionCount);
	if (requiredExtensions == nullptr) {
		fprintf(stderr, "Failed allocating memory for extensions\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto failure;
	}
	for (uint32_t i = 0; i < extensionCount; i++) {
		requiredExtensions[i] = glfwExtensions[i];
	}
	#endif

	// Checking for extension support
	{
		uint32_t supportedExtensionsNum = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, nullptr);
		VkExtensionProperties supportedExtensions[supportedExtensionsNum];
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, supportedExtensions);
		printf("The following extensions are supported:\n");

		for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
			printf("%s, ", supportedExtensions[j].extensionName);
		}
		printf("\n");

		// Checking if required extensions are supported
		for (uint32_t i = 0; i < extensionCount; i++) {
			bool found = false;
			for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
				if (strcmp(requiredExtensions[i], supportedExtensions[j].extensionName) == 0) {
					found = true;
				}
			}
			if (!found) {
				fprintf(stderr, "Required extension %s is not supported\n", requiredExtensions[i]);
				result = VK_ERROR_EXTENSION_NOT_PRESENT;
				goto failure;
			}
		}
	}

	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = requiredExtensions;
	createInfo.enabledLayerCount = 0;

	result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		fprintf(stderr, "vkCreateInstanced failed with exit code: %d\n", result);
		goto failure;
	}

	return result;

	failure:
		if (requiredExtensions != nullptr) {
			free(requiredExtensions);
			requiredExtensions = nullptr;
		}
		return result;
}

static void initVulkan() {
	createInstance();
}

static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

static void cleanUp() {
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanUp();
}

int main() {
	run();
	return EXIT_SUCCESS;
}
