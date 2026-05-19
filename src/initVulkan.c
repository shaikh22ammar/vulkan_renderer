#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef MAC_OS
#include <vulkan/vulkan_beta.h>
#endif

#include "rendererErrors.h"


/* enable validation layers if in debug mode */
#ifdef NDEBUG 
constexpr VkBool32 enableValidationLayers = VK_FALSE;
#else
constexpr VkBool32 enableValidationLayers = VK_TRUE;
#endif

static constexpr uint32_t validationLayersCount = 1;
static const char *validationLayers[validationLayersCount] = {"VK_LAYER_KHRONOS_validation"};
static uint32_t checkValidationLayerSupport() {
	/* Checks if the validation layers in the 
	 * global validationLayers is supported */
	/* Returns the index of the layer not present */
	VkResult result;

	uint32_t supportedLayersNum = 0;
	result = vkEnumerateInstanceLayerProperties(&supportedLayersNum, nullptr);
	handleVulkanError(result, "vkEnumerateInstanceLayerProperties", true);
	VkLayerProperties supportedLayers[supportedLayersNum];
	result = vkEnumerateInstanceLayerProperties(&supportedLayersNum, supportedLayers);
	handleVulkanError(result, "vkEnumerateInstanceLayerProperties", true);

	uint32_t i;
	for (i = 0; i < validationLayersCount; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedLayersNum; j++) {
			if (strcmp(validationLayers[i], supportedLayers[j].layerName) == 0) {
				found = VK_TRUE;
				result = VK_SUCCESS;
				break;
			}
		}
		if (!found) {
			break;
		}
	}

	return i;
}

static uint32_t requiredInstanceExtensionsCount = 0;
static const char **requiredInstanceExtensions = nullptr;
static uint32_t checkExtensionSupport() {
	/* Checks if the required instance extensions are supported in the instance 
	 * Exits if not supported */
	/* Returns the index of the extension not present */
	VkResult result;

	uint32_t supportedExtensionsNum = 0;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, nullptr);
	handleVulkanError(result, "vkEnumerateInstanceExtensionProperties", true);
	VkExtensionProperties supportedExtensions[supportedExtensionsNum];
	result = vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, supportedExtensions);
	handleVulkanError(result, "vkEnumerateInstanceExtensionProperties", true);

	uint32_t i;
	for (i = 0; i < requiredInstanceExtensionsCount; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
			if (strcmp(requiredInstanceExtensions[i], supportedExtensions[j].extensionName) == 0) {
				found = VK_TRUE;
				break;
			}
		}
		if (!found) {
			break;
		}
	}

	return i;
}

extern VkInstance instance;
static VkResult createInstance() {
	/* Creates an Instance
	 * First, checks if the given validation layers are supported (provided NDEBUG is not defined),
	 * Then checks for the required instance extensions (these are glfw extensions,
	 * and portability extensions for MacOS) */
	VkResult result = VK_SUCCESS;
	uint32_t unsupportedLayer;

	// Checking validation layer support
#ifndef NDEBUG
	unsupportedLayer = checkValidationLayerSupport();
	if (unsupportedLayer < validationLayersCount) {
		fprintf(stderr, "INIT ERROR: validation layer %s not present\n", validationLayers[unsupportedLayer]);
		exit(RENDERER_ERROR_VULKAN);
	}
#endif

	// Finding required extensions for glfw
	uint32_t glfwExtensionsCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	requiredInstanceExtensionsCount = glfwExtensionsCount;
#ifndef NDEBUG
	requiredInstanceExtensionsCount++;
#endif
#ifdef MAC_OS 
	requiredInstanceExtensionsCount++;
#endif
	requiredInstanceExtensions = malloc((sizeof(const char *)) * requiredInstanceExtensionsCount);
	if (!requiredInstanceExtensions) {
		fprintf(stderr, "ERROR: Failed allocating memory for extensions\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto failure;
	}
	for (uint32_t i = 0; i < glfwExtensionsCount; i++) {
		requiredInstanceExtensions[i] = glfwExtensions[i];
	}
#ifdef MAC_OS
	if (!enableValidationLayers)
		requiredInstanceExtensions[requiredInstanceExtensionsCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	else
		requiredInstanceExtensions[requiredInstanceExtensionsCount - 2] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
#endif
#ifndef NDEBUG
	requiredInstanceExtensions[requiredInstanceExtensionsCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif
	puts("The following extensions are required:");
	for (uint32_t i = 0; i < requiredInstanceExtensionsCount; i++) {
		printf("%s, ", requiredInstanceExtensions[i]);
	}
	puts("");
	// Checking for extension support	
	unsupportedLayer = checkExtensionSupport();
	if (unsupportedLayer < requiredInstanceExtensionsCount) {
		fprintf(stderr, "INIT ERROR: extension %s not present\n", requiredInstanceExtensions[unsupportedLayer]);
		exit(RENDERER_ERROR_VULKAN);
	}
	puts("");

	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = requiredInstanceExtensionsCount;
	createInfo.ppEnabledExtensionNames = requiredInstanceExtensions;
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
	if (result < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreateInstance failed with exit code: %d\n", result);
		goto failure;
	}
	free(requiredInstanceExtensions);
	requiredInstanceExtensions = nullptr;
	return result;

failure:
	if (requiredInstanceExtensions) {
		free(requiredInstanceExtensions);
		requiredInstanceExtensions = nullptr;
	}
	return result;
}

/* Debug */
static VkResult createDebugUtilsMessengerEXT(
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }


}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData) {

	fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

	return VK_FALSE;
}

extern VkDebugUtilsMessengerEXT debugMessenger;
static VkResult setupDebugMessenger() {
	if (!enableValidationLayers) return VK_SUCCESS;
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional
	
	return createDebugUtilsMessengerEXT(&createInfo, nullptr, &debugMessenger);
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			SURFACE CREATION
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern GLFWwindow *window;
extern VkSurfaceKHR surface;
static VkResult createSurface() {
	/* The window system Vulkan is abstracted away by a VkSurface. */
	VkResult result;
	if ((result = glfwCreateWindowSurface(instance, window, nullptr, &surface)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: glfwCreateWindowSurface exited with error code %d", result);
	}
	return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			PHYSICAL DEVICE CHOICE
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
// The compile-time constance number of device extensions required
static constexpr uint32_t __requiredDeviceExtensionsCount = 1;
// the runtime dependent number of device extensions required
// (protability extensions for non-Vulkan compatible devices)
static uint32_t requiredDeviceExtensionsCount = __requiredDeviceExtensionsCount;
static const char *requiredDeviceExtensions[__requiredDeviceExtensionsCount + 1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};
static uint32_t isDeviceSuitable(VkPhysicalDevice currPhysicalDevice) {
	/* Checks if currPhysicalDevice is suitable.
	 * A device is considered suitable if:
	 * It supports at least API version 1.3,
	 * It supports dynamic rendering and dynamic state extensions,
	 * It supports the required device extensions,
	 * If it has a queue family that supports both graphics and present command. 
	 * Also, if a device supports portability extension is checked,
	 * if so, it must be activated */
	
	// check if device supports api version 1.3 or higher
	VkPhysicalDeviceProperties2 deviceProperies = {0};
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(currPhysicalDevice, &deviceProperies);
	VkBool32 isApiVersionSupported = deviceProperies.properties.apiVersion >= VK_API_VERSION_1_3;

	// check if dynamic rendering and dynamic state features extensions are supported
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceVulkanExtendedDynamicStateFeaturesEXT;
	deviceVulkanExtendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	deviceVulkanExtendedDynamicStateFeaturesEXT.pNext = nullptr;

	VkPhysicalDeviceVulkan13Features deviceVulkan13Features;
	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = &deviceVulkanExtendedDynamicStateFeaturesEXT;

	VkPhysicalDeviceVulkan11Features deviceVulkan11Features;
	deviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	deviceVulkan11Features.pNext = &deviceVulkan13Features;

	VkPhysicalDeviceFeatures2 deviceFeatures;
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &deviceVulkan11Features;

	vkGetPhysicalDeviceFeatures2(currPhysicalDevice, &deviceFeatures);
	VkBool32 isAllFeaturesSupported = deviceVulkan11Features.shaderDrawParameters 
		&& deviceVulkan13Features.dynamicRendering 
		&& deviceVulkan13Features.synchronization2
		&& deviceVulkanExtendedDynamicStateFeaturesEXT.extendedDynamicState;

	// check if requireds extensions are supported
	VkBool32 isPortabilitySubsetRequired = VK_FALSE;
	VkBool32 isAllDeviceExtensionsSupported = VK_TRUE;
	uint32_t supportedDeviceExtensionPropertiesCount;
	vkEnumerateDeviceExtensionProperties(currPhysicalDevice, nullptr, &supportedDeviceExtensionPropertiesCount, nullptr);
	VkExtensionProperties *supportedDeviceExtensionProperties 
		= malloc(sizeof(VkExtensionProperties)*supportedDeviceExtensionPropertiesCount);
	if (!supportedDeviceExtensionProperties) {
		fprintf(stderr, 
			"ERROR: Failed allocating memory for finding supported extensions of device: %d\n",
			deviceProperies.properties.deviceID);
		return VK_FALSE;
	}
	vkEnumerateDeviceExtensionProperties(
			currPhysicalDevice, 
			nullptr, 
			&supportedDeviceExtensionPropertiesCount, 
			supportedDeviceExtensionProperties
			);
	for(uint32_t i = 0; i < __requiredDeviceExtensionsCount + 1; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedDeviceExtensionPropertiesCount; j++) {
			if (!strcmp(requiredDeviceExtensions[i], supportedDeviceExtensionProperties[j].extensionName)) {
				found = VK_TRUE;
				if (i == __requiredDeviceExtensionsCount) {
					// Portability device extension found, hence required
					requiredDeviceExtensionsCount++;
					isPortabilitySubsetRequired = VK_TRUE;
				}
				break;
			}
		}
		if (!found && i < __requiredDeviceExtensionsCount) {
			isAllDeviceExtensionsSupported = VK_FALSE;
			break;
		}
	}
	free(supportedDeviceExtensionProperties);
	supportedDeviceExtensionProperties = nullptr;

	// check if there is queue family has the required queue commands: graphics and present
	VkBool32 isAllQueueCommandsSupported = VK_FALSE;
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties2(currPhysicalDevice, &queueFamiliesCount, nullptr);
	VkQueueFamilyProperties2 *queueFamiliesProperties = malloc(sizeof(VkQueueFamilyProperties2)*queueFamiliesCount);
	if (!queueFamiliesProperties) {
		fprintf(stderr, 
			"ERROR: Failed allocating memory for finding queue families of device: %d\n",
			deviceProperies.properties.deviceID);
		return VK_FALSE;
	}
	for (uint32_t i = 0; i < queueFamiliesCount; i++) {
		queueFamiliesProperties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		queueFamiliesProperties[i].pNext = nullptr;
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(currPhysicalDevice, &queueFamiliesCount, queueFamiliesProperties);
	for (uint32_t i = 0; i < queueFamiliesCount; i++) {
		VkQueueFamilyProperties2 queueFamily = queueFamiliesProperties[i];
		VkBool32 existsGraphicsFamily = VK_FALSE;
		VkBool32 existsPresentFamily = VK_FALSE;
		if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			existsGraphicsFamily = VK_TRUE;
		}
		VkResult result = VK_SUCCESS;
		if ((result = vkGetPhysicalDeviceSurfaceSupportKHR(currPhysicalDevice, i, surface, &existsPresentFamily)) < VK_SUCCESS) {
			fprintf(stderr, "ERROR: vkGetPhysicalDeviceSurfaceSupportKHR failed with error code: %d\n", result);	
			return VK_FALSE;
		}
		isAllQueueCommandsSupported = existsGraphicsFamily && existsPresentFamily;
	}
	free(queueFamiliesProperties);
	queueFamiliesProperties = nullptr;

	uint32_t result = 0U;
	result |= isApiVersionSupported;
	result |= isAllQueueCommandsSupported << 1;
	result |= isAllFeaturesSupported << 2;
	result |= isAllDeviceExtensionsSupported << 3;
	result |= isPortabilitySubsetRequired << 4;

	return result;

}

extern VkPhysicalDevice physicalDevice;
static VkResult pickPhysicalDevice() {
	uint32_t physicalDevicesCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr);
	if (physicalDevicesCount == 0) {
		fprintf(stderr, "ERROR: Unable to find any GPU with Vulkan support\n");
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	VkPhysicalDevice availablePhysicalDevices[physicalDevicesCount];
	vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, availablePhysicalDevices);

	puts("Found devices:");
	for (uint32_t i = 0; i < physicalDevicesCount; i++) {
		VkPhysicalDevice currPhysicalDevice = availablePhysicalDevices[i];
		VkPhysicalDeviceProperties2 currPhysicalDeviceProperies;
		currPhysicalDeviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		currPhysicalDeviceProperies.pNext = nullptr;
		vkGetPhysicalDeviceProperties2(currPhysicalDevice, &currPhysicalDeviceProperies);
		printf("%d: %s\n", currPhysicalDeviceProperies.properties.deviceID, currPhysicalDeviceProperies.properties.deviceName);		
		if (isDeviceSuitable(currPhysicalDevice) >= 0x1F && physicalDevice == VK_NULL_HANDLE) {
			physicalDevice = currPhysicalDevice;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		fprintf(stderr, "ERROR: Unable to find any suitable GPU");
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	VkPhysicalDeviceProperties2 chosenDeviceProperies;
	chosenDeviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	chosenDeviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &chosenDeviceProperies);
	printf("Picked device %d: %s\n", chosenDeviceProperies.properties.deviceID, chosenDeviceProperies.properties.deviceName);
	puts("");
	return VK_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			LOGICAL DEVICE CREATION
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern VkDevice device;
extern VkQueue queue; 
extern uint32_t queueFamilyIndex;
static VkResult createLogicalDevice() {
	/* Creates the logical device.
	 * Finds the queue family that supports both graphics and present command, initializes queue with that
	 * Note, if device requires portability extension, requiredDeviceExtensionsCount would already be incremented by now
	 * to reflect this */
	VkResult result = VK_SUCCESS;

	// Finding the queueFamilyIndex that supports graphics and presentation commands simultanteously
	VkPhysicalDeviceProperties2 deviceProperies;
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperies);

	VkBool32 queueFamilyFound = VK_FALSE;
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, nullptr);
	VkQueueFamilyProperties2 *queueFamiliesProperties = malloc(sizeof(VkQueueFamilyProperties2)*queueFamiliesCount);
	if (!queueFamiliesProperties) {
		fprintf(stderr, 
			"ERROR: Failed allocating memory for finding queue families of device: %d\n",
			deviceProperies.properties.deviceID);
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	for (uint32_t i = 0; i < queueFamiliesCount; i++) {
		queueFamiliesProperties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		queueFamiliesProperties[i].pNext = nullptr;
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, queueFamiliesProperties);
	for (uint32_t i = 0; i < queueFamiliesCount; i++) {
		VkQueueFamilyProperties2 queueFamily = queueFamiliesProperties[i];
		VkBool32 existsGraphicsCommand = VK_FALSE;
		VkBool32 existsPresentCommand = VK_FALSE;
		if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			existsGraphicsCommand = VK_TRUE;
		}
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &existsPresentCommand);
		if (existsGraphicsCommand && existsPresentCommand) {
			queueFamilyIndex = i;
			queueFamilyFound = VK_TRUE;
		}
	}
	free(queueFamiliesProperties);
	queueFamiliesProperties = nullptr;

	if (!queueFamilyFound) {
		result = VK_ERROR_INITIALIZATION_FAILED;
		fprintf(stderr, "ERROR: No queue family found that supports both graphics and present commands.\n");
		return result;
	}

	// Specifying queues
	float queuePriority = 0.5f;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {0};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
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
	deviceVulkan13Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceVulkan11Features deviceVulkan11Features = {0};
	deviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	deviceVulkan11Features.pNext = &deviceVulkan13Features;
	deviceVulkan11Features.shaderDrawParameters = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures = {0};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &deviceVulkan11Features;

	// Creating logical device
	VkDeviceCreateInfo deviceCreateInfo = {0};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &deviceFeatures;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = requiredDeviceExtensionsCount;
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions;

	if ((result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreateDevice failed with exit code: %d\n", result);
		return result;
	}

 	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
	return result;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			SWAP CHAIN CREATION
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	

extern VkSwapchainKHR swapChain;
extern VkImage *swapChainImages;
extern uint32_t swapChainImagesCount;
extern VkSurfaceFormatKHR swapChainSurfaceFormat;
extern VkExtent2D swapChainExtent;
VkResult createSwapChain() {
	/* Swap chain creation
	 *
	 * Surface is created.
	 * Format B8G8R8_SRGB, color space SRGB Nonlinear
	 * if supported, otherwise falls back to format that comes first.
	 *
	 * Present mode is chosen as Mailbox, but falls back to fifo relaxed if not supported,
	 * unless debug mode is on, in which case, it's always fifo.
	 * */


	// Choosing surface format
	VkResult result = VK_SUCCESS;
	/* We need to choose the format of the surface: colorspace and bit depth format */
	uint32_t surfaceFormatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr);
	VkSurfaceFormatKHR *surfaceFormats = malloc(sizeof(VkSurfaceFormatKHR)*surfaceFormatsCount);
	if (!surfaceFormats) {
		fprintf(stderr, "ERROR: Failed allocating memory for querying surface format\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		return result;
	}
	if((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceFormats)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetPhysicalDeviceSurfaceFormatsKHR exited with error code %d\n", result);
		return result;
	}
	VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
	for (uint32_t i = 0; i < surfaceFormatsCount; i++) {
		if (surfaceFormats[i].format == VK_FORMAT_B8G8R8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = surfaceFormats[i];
			break;
		}
	}
	free(surfaceFormats);
	surfaceFormats = nullptr;

	// Choosing present modes
	/* Present modes state what should be done when application and monitor refresh rate is out of sync */
	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);
	VkPresentModeKHR *presentModes = malloc(sizeof(VkPresentModeKHR)*presentModesCount);
	if (!presentModes) {
		fprintf(stderr, "ERROR: Failed allocating memory for querying present modes\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		return result;
	}
	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, presentModes)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetPhysicalDeviceSurfacePresentModesKHR exited with error code %d\n", result);
		return result;
	}
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
#ifdef NDEBUG
	for (uint32_t i = 0; i < presentModesCount; i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModes[i];
			break;
		}
	}
#endif
	free(presentModes);
	presentModes = nullptr;

	// Choosing swap extent
	/* We need to state the extent of the surface (dimension) 
	 * By default, it would be the extent of the window,
	 * but if the width is ~0U, it needs to be specified */
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetPhysicalDeviceSurfaceCapabilitiesKHR exited with error code %d\n", result);
		return result;
	}
	VkExtent2D surfaceExtent = surfaceCapabilities.currentExtent;
	if (true) {//surfaceCapabilities.currentExtent.width == ~0U) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		w = (uint32_t) w > surfaceCapabilities.maxImageExtent.width ? surfaceCapabilities.maxImageExtent.width : w;
		w = (uint32_t) w < surfaceCapabilities.minImageExtent.width ? surfaceCapabilities.minImageExtent.width : w;
		surfaceExtent.width = w;
		h = (uint32_t) h > surfaceCapabilities.maxImageExtent.height ? surfaceCapabilities.maxImageExtent.height : h;
		h = (uint32_t) h < surfaceCapabilities.minImageExtent.height ? surfaceCapabilities.minImageExtent.height : h;
		surfaceExtent.height = h;
	}
	/* We need to specify the minimum number of images that would be
	 * buffered in the swapchain.
	 * Excepting edge cases, we choose 3 */
	uint32_t minImageCount = surfaceCapabilities.minImageCount < 3U ? surfaceCapabilities.minImageCount : 3U;
	if (0 < surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount < 3U)
		minImageCount = surfaceCapabilities.maxImageCount;

	// Creating the swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {0};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.pNext = nullptr;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.minImageCount = minImageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = surfaceExtent;
	swapChainCreateInfo.imageArrayLayers = 1U;
				/* Specifies the number of stereoscopic views. 
				 * For our purpose it is 1 */
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 
				/* specifies that the image can be 
				 * used to create a VkImageView suitable for use as a color */
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				/* specifies that access to any range or image subresource 
				 * of the object will be exclusive to a single queue family at a time. */
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
				/* No transformation */
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
				/* specifies whether the Vulkan implementation is allowed to
				 * discard rendering operations that affect regions of the surface that are not visible. */
	swapChainCreateInfo.oldSwapchain = nullptr;

	if ((result = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreateSwapchainKHR exited with error code %d\n", result);
		return result;
	}

	// Creating swap chain images array
	vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, nullptr);
	swapChainImages = malloc(sizeof(VkImage)*swapChainImagesCount);
	if (!swapChainImages) {
		fprintf(stderr, "ERROR: Failed allocating memory for swap chain images.\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		return result;
	}
	if ((result = vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, swapChainImages)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetSwapchainImagesKHR exited with error code %d\n", result);
		return result;
	}
	swapChainSurfaceFormat = surfaceFormat;
	swapChainExtent = surfaceExtent;

	return result;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			IMAGE VIEW CREATION
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern VkImageView *swapChainImageViews;
VkResult createImageViews() {
	/* Image view determine how to actually interpret the image
	 * irregardless of its format. For our purpose of swapchains,
	 * we want interpretation to be same as the format */
	VkResult result = VK_SUCCESS;
	VkImageViewCreateInfo imageViewCreateInfo = {0};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = swapChainSurfaceFormat.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				/* This means we only want the color of the image as 
				 * opposed to depth and other aspects */
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	
	swapChainImageViews = malloc(sizeof(VkImageView)*swapChainImagesCount);
	for (uint32_t i = 0; i < swapChainImagesCount; i++) {
		imageViewCreateInfo.image = swapChainImages[i];
		if ((result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, swapChainImageViews + i)) < VK_SUCCESS) {
			fprintf(stderr, "ERROR: vkCreateImageView exited with error code %d\n", result);
			exit(result);
		}
	}
	return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void initVulkan() {
	constexpr int numFunctions = 7;
	VkResult (*functionsToCall[numFunctions])(void) = {
		createInstance,
		setupDebugMessenger,
		createSurface,
		pickPhysicalDevice, 
		createLogicalDevice, 
		createSwapChain, 
		createImageViews
	};

	const char *functionsToCallNames[numFunctions] = {
		"createInstance",
		"setupDebugMessenger",
		"createSurface",
		"pickPhysicalDevice", 
		"createLogicalDevice", 
		"createSwapChain", 
		"createImageViews"
	};

	for (int i = 0; i < numFunctions; i++) {
		handleVulkanError(functionsToCall[i](), functionsToCallNames[i], true);
	}
}
