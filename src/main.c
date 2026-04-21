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


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			WINDOW CREATION
 *
 *	A window is created using GLFW with no API option to prevent
 *	it from opening an OpenGL contex
 *
 *	Global variables:
 *		WIDTH
 *		HEIGHT
 *		window
 *
 *	Functions:
 *		initWindow()
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
GLFWwindow *window;
static bool initWindow() {
	glfwInit();
	// prevent GLFW from loading openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Unable to create GLFW window\n");
		return false;
	}
	return true;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			INSTANCE CREATION
 *
 *	A vulkan instance is created.
 *	Validation layers are checked if program is in debug mode.
 *	Required extensions are checked for GLFW and MoltenVk if program is in MAC_OS mode
 *
 *	Global variables:
 *		enableValidationLayers
 *		validationLayers[validationLayersCount]
 *		requiredExtensions[requiredExtensionsCount]
 *		instance
 *
 *	Functions:
 *		checkExtensionSupport()
 *		createInstance()
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif
static constexpr uint32_t validationLayersCount = 1;
static const char *validationLayers[validationLayersCount] = {"VK_LAYER_KHRONOS_validation"};
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

static uint32_t requiredExtensionsCount = 0;
static const char **requiredExtensions = nullptr;
static bool checkExtensionSupport() {
	bool allSupported = true;

	uint32_t supportedExtensionsNum = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, nullptr);
	VkExtensionProperties supportedExtensions[supportedExtensionsNum];
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, supportedExtensions);
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

VkInstance instance;
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
	puts("The following extensions are required:");
	for (uint32_t i = 0; i < requiredExtensionsCount - 1; i++) {
		requiredExtensions[i] = glfwExtensions[i];
		printf("%s, ", requiredExtensions[i]);
	}
	#ifdef MAC_OS
	requiredExtensions[requiredExtensionsCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	printf("%s\n", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	#endif
	// Checking for extension support
	if (!checkExtensionSupport()) {
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto failure;
	}
	puts("");

	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			PHYSICAL DEVICE CHOICE
 *
 * 	A physical device is chosen.
 *
 *	Global variables:
 * 		requiredDeviceExtensions
 *		requiredGraphicsCommandsQueueFamilyIndex
 * 		physicalDevice
 *
 *	Functions:
 *		isDeviceSuitable()
 *		pickPhysicalDevice()
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
constexpr uint32_t requiredDeviceExtensionsCount = 1;
const char *requiredDeviceExtensions[requiredDeviceExtensionsCount + 1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};
bool isPortabilitySubsetRequired = false;
struct {uint32_t graphicsFamily;} requiredGraphicsCommandsQueueFamilyIndex;
bool isDeviceSuitable(VkPhysicalDevice device) {
	// check if device supports api version 1.3 or higher
	VkPhysicalDeviceProperties2 deviceProperies;
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(device, &deviceProperies);
	bool isApiVersionSupported = deviceProperies.properties.apiVersion >= VK_API_VERSION_1_3;

	// check if dynamic rendering and extended dynamic state features are supported
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceVulkanExtendedDynamicStateFeaturesEXT;
	deviceVulkanExtendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	deviceVulkanExtendedDynamicStateFeaturesEXT.pNext = nullptr;

	VkPhysicalDeviceVulkan13Features deviceVulkan13Features;
	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = &deviceVulkanExtendedDynamicStateFeaturesEXT;

	VkPhysicalDeviceFeatures2 deviceFeatures;
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &deviceVulkan13Features;

	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);
	bool isAllFeaturesSupported = deviceVulkan13Features.dynamicRendering 
		&& deviceVulkanExtendedDynamicStateFeaturesEXT.extendedDynamicState;

	// check if requireds extensions are supported
	bool isAllDeviceExtensionsSupported = true;
	uint32_t supportedDeviceExtensionPropertiesCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedDeviceExtensionPropertiesCount, nullptr);
	VkExtensionProperties *supportedDeviceExtensionProperties 
		= malloc(sizeof(VkExtensionProperties)*supportedDeviceExtensionPropertiesCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedDeviceExtensionPropertiesCount, supportedDeviceExtensionProperties);
	for(uint32_t i = 0; i < requiredDeviceExtensionsCount; i++) {
		bool found = false;
		for (uint32_t j = 0; j < supportedDeviceExtensionPropertiesCount; j++) {
			if (!strcmp(requiredDeviceExtensions[i], supportedDeviceExtensionProperties[j].extensionName)) {
				found = true;
				break;
			}
			if (!strcmp(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, supportedDeviceExtensionProperties[j].extensionName)) {
				isPortabilitySubsetRequired = true;
			} 
		}
		if (!found) {
			isAllDeviceExtensionsSupported = false;
			break;
		}
	}
	free(supportedDeviceExtensionProperties);
	supportedDeviceExtensionProperties = nullptr;

	// check if there is queue family supporting graphics commands
	bool existsGraphicsFamily = false;
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamiliesCount, nullptr);
	VkQueueFamilyProperties2 *queueFamiliesProperties = malloc(sizeof(VkQueueFamilyProperties2)*queueFamiliesCount);
	for (uint32_t i = 0; i < queueFamiliesCount; i++) {
		queueFamiliesProperties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		queueFamiliesProperties[i].pNext = nullptr;
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamiliesCount, queueFamiliesProperties);
	for (uint32_t i = 0; i < queueFamiliesCount; i++) {
		VkQueueFamilyProperties2 queueFamily = queueFamiliesProperties[i];
		/*printf("Queue index: %u, queue flags: %B, queueCount: %d\n", 
				i, queueFamily.queueFamilyProperties.queueFlags, queueFamily.queueFamilyProperties.queueCount);
		*/
		if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			existsGraphicsFamily = true;
			requiredGraphicsCommandsQueueFamilyIndex.graphicsFamily = i;
			break;
		}
	}
	free(queueFamiliesProperties);
	queueFamiliesProperties = nullptr;

	return isApiVersionSupported && existsGraphicsFamily && isAllFeaturesSupported && isAllDeviceExtensionsSupported;
}

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static bool pickPhysicalDevice() {
	uint32_t physicalDevicesCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr);
	if (physicalDevicesCount == 0) {
		fprintf(stderr, "Unable to find any GPU with Vulkan support\n");
		return false;
	}
	VkPhysicalDevice availablePhysicalDevices[physicalDevicesCount];
	vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, availablePhysicalDevices);

	puts("Found devices:");
	for (uint32_t i = 0; i < physicalDevicesCount; i++) {
		VkPhysicalDevice device = availablePhysicalDevices[i];
		VkPhysicalDeviceProperties2 deviceProperies;
		deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperies.pNext = nullptr;
		vkGetPhysicalDeviceProperties2(device, &deviceProperies);
		printf("%d: %s\n", deviceProperies.properties.deviceID, deviceProperies.properties.deviceName);		
		if (isDeviceSuitable(device) && physicalDevice == VK_NULL_HANDLE) {
			physicalDevice = device;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		fprintf(stderr, "Unable to find any suitable GPU");
		return false;
	}
	VkPhysicalDeviceProperties2 deviceProperies;
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperies);
	printf("Picked device %d: %s\n", deviceProperies.properties.deviceID, deviceProperies.properties.deviceName);
	puts("");
	return true;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			LOGICAL DEVICE CREATION
 *
 * 	A logical device is created.
 *
 *	Global variables:
 *		device
 *
 *	Functions:
 *		createLogicalDevice()
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
VkDevice device;
VkResult createLogicalDevice() {
	// Specifying queues
	float queuePriority = 0.5f;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {0};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.queueFamilyIndex = requiredGraphicsCommandsQueueFamilyIndex.graphicsFamily;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
	
	// Specifying features
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceVulkanExtendedDynamicStateFeaturesEXT = {0};
	deviceVulkanExtendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	deviceVulkanExtendedDynamicStateFeaturesEXT.pNext = nullptr;
	deviceVulkanExtendedDynamicStateFeaturesEXT.extendedDynamicState = VK_TRUE;

	VkPhysicalDeviceVulkan13Features deviceVulkan13Features = {0};
	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = &deviceVulkanExtendedDynamicStateFeaturesEXT;
	deviceVulkan13Features.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures = {0};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &deviceVulkan13Features;

	// Creating logical device
	VkDeviceCreateInfo deviceCreateInfo = {0};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &deviceFeatures;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = requiredDeviceExtensionsCount + isPortabilitySubsetRequired;
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions;
	return vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
}

static bool initVulkan() {
	if (createInstance() != VK_SUCCESS) return false;
	if (!pickPhysicalDevice()) return false;
	if (createLogicalDevice() != VK_SUCCESS) return false;
	return true;
}

static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

static void cleanUp() {
	if (device) vkDestroyDevice(device, nullptr);
	if (instance) vkDestroyInstance(instance, nullptr);
	if (window) glfwDestroyWindow(window);
	glfwTerminate();
}

static void run() {
	if (!initWindow()) goto cleanup;
	if (!initVulkan()) goto cleanup;
	mainLoop();
	cleanup:
		cleanUp();
}

int main() {
	run();
	return EXIT_SUCCESS;
}
