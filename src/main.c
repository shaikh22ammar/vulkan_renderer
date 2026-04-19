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

uint32_t requiredExtensionsCount = 0;
const char **requiredExtensions = nullptr;

constexpr uint32_t validationLayersCount = 1;
const char *validationLayers[validationLayersCount] = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

static void initWindow() {
	glfwInit();
	// prevent GLFW from loading openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

static bool checkExtensionSupport() {
	bool allSupported = true;

	uint32_t supportedExtensionsNum = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, nullptr);
	VkExtensionProperties supportedExtensions[supportedExtensionsNum];
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, supportedExtensions);
	printf("The following extensions are supported:\n");
	for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
		printf("%s, ", supportedExtensions[j].extensionName);
	}
	printf("\n");

	for (uint32_t i = 0; i < requiredExtensionsCount; i++) {
		bool found = false;
		for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
			if (strcmp(requiredExtensions[i], supportedExtensions[j].extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Required extension %s is not supported\n", requiredExtensions[i]);
			allSupported = false;
		}
	}

	return allSupported;
}

static bool checkValidationLayerSupport() {
	bool allSupported = true;

	uint32_t supportedLayersNum = 0;
	vkEnumerateInstanceLayerProperties(&supportedLayersNum, nullptr);
	VkLayerProperties supportedLayers[supportedLayersNum];
	vkEnumerateInstanceLayerProperties(&supportedLayersNum, supportedLayers);

	for (uint32_t i = 0; i < validationLayersCount; i++) {
		bool found = false;
		for (uint32_t j = 0; j < supportedLayersNum; j++) {
			if (strcmp(validationLayers[i], supportedLayers[j].layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Validation layer %s is not supported\n", validationLayers[i]);
			allSupported = false;
		}
	}

	return allSupported;
}

static VkInstance instance;
static VkResult createInstance() {
	VkResult result = VK_SUCCESS;

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		return VK_ERROR_LAYER_NOT_PRESENT;
	}

	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	requiredExtensionsCount = glfwExtensionCount;
	#ifdef MAC_OS
	// one more portability extension is required for MoltenVk
	requiredExtensionsCount++;
	#endif
	requiredExtensions = malloc((sizeof *glfwExtensions) * requiredExtensionsCount);
	if (requiredExtensions == nullptr) {
		fprintf(stderr, "Failed allocating memory for extensions\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto failure;
	}
	for (uint32_t i = 0; i < requiredExtensionsCount - 1; i++) {
		requiredExtensions[i] = glfwExtensions[i];
	}
	#ifdef MAC_OS
	requiredExtensions[requiredExtensionsCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	#endif
	// Checking for extension support
	if (!checkExtensionSupport()) {
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto failure;
	}
	puts("All required extensions are supported");

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
	createInfo.enabledExtensionCount = requiredExtensionsCount;
	createInfo.ppEnabledExtensionNames = requiredExtensions;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validationLayersCount;
		createInfo.ppEnabledLayerNames = validationLayers;
	} else {
		createInfo.enabledLayerCount = 0;
	}
	#ifdef MAC_OS
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	#endif

	result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "vkCreateInstance failed with exit code: %d\n", result);
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
