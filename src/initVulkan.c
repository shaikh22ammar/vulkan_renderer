/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			VULKAN INTIALIZATION
 *
 * 		Baisc vulkan boilerplate: instance and device creation.
 *
 * 		The following external variables are initialized here:
 * 			instance
 * 			surface
 * 			physicalDevice
 * 			device
 * 			queue
 *
 * 		when main calls the external function initVulkan() defined here.
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	

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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			INSTANCE CREATION
 *
 *	A vulkan instance is created.
 *	Validation layers are checked if program is in debug mode.
 *	Required extensions are checked for GLFW and MoltenVk if program is in MAC_OS mode
 *
 *	Global variables:
 *		instance (extern)
 *
 *	Functions:
 *		checkExtensionSupport()
 *		createInstance()
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	

#ifdef NDEBUG
constexpr VkBool32 enableValidationLayers = VK_FALSE;
#else
constexpr VkBool32 enableValidationLayers = VK_TRUE;
#endif
static constexpr uint32_t validationLayersCount = 1;
static const char *validationLayers[validationLayersCount] = {"VK_LAYER_KHRONOS_validation"};
static VkResult checkValidationLayerSupport() {
	VkResult result = VK_SUCCESS;

	uint32_t supportedLayersNum = 0;
	vkEnumerateInstanceLayerProperties(&supportedLayersNum, nullptr);
	VkLayerProperties supportedLayers[supportedLayersNum];
	vkEnumerateInstanceLayerProperties(&supportedLayersNum, supportedLayers);

	for (uint32_t i = 0; i < validationLayersCount; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedLayersNum; j++) {
			if (strcmp(validationLayers[i], supportedLayers[j].layerName) == 0) {
				found = VK_TRUE;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "ERROR: Validation layer %s is not supported\n", validationLayers[i]);
			result = VK_ERROR_LAYER_NOT_PRESENT;
		}
	}

	return result;
}

static uint32_t requiredInstanceExtensionsCount = 0;
static const char **requiredInstanceExtensions = nullptr;
static VkResult checkExtensionSupport() {
	VkResult result = VK_SUCCESS;

	uint32_t supportedExtensionsNum = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, nullptr);
	VkExtensionProperties supportedExtensions[supportedExtensionsNum];
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, supportedExtensions);
	for (uint32_t i = 0; i < requiredInstanceExtensionsCount; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
			if (strcmp(requiredInstanceExtensions[i], supportedExtensions[j].extensionName) == 0) {
				found = VK_TRUE;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "ERROR: Required extension %s is not supported\n", requiredInstanceExtensions[i]);
			result = VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	return result;
}

extern VkInstance instance;
static VkResult createInstance() {
	VkResult result = VK_SUCCESS;

	// Checking validation layer support
	if (enableValidationLayers) {
		VkResult result = checkValidationLayerSupport();
		if (result < VK_SUCCESS) return result;
	}

	// Finding required extensions
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	requiredInstanceExtensionsCount = glfwExtensionCount;
	#ifdef MAC_OS // portability extension is required for MoltenVk
	requiredInstanceExtensionsCount++;
	#endif
	requiredInstanceExtensions = malloc((sizeof *glfwExtensions) * requiredInstanceExtensionsCount);
	if (!requiredInstanceExtensions) {
		fprintf(stderr, "ERROR: Failed allocating memory for GLFW extensions\n");
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto failure;
	}
	puts("The following extensions are required:");
	for (uint32_t i = 0; i < requiredInstanceExtensionsCount - 1; i++) {
		requiredInstanceExtensions[i] = glfwExtensions[i];
		printf("%s, ", requiredInstanceExtensions[i]);
	}
	#ifdef MAC_OS
	requiredInstanceExtensions[requiredInstanceExtensionsCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	printf("%s\n", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	#endif

	// Checking for extension support
	if ((result = checkExtensionSupport()) < VK_SUCCESS) {
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
	if (requiredInstanceExtensions != nullptr) {
		free(requiredInstanceExtensions);
		requiredInstanceExtensions = nullptr;
	}
	return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			SURFACE CREATION
 *
 *		A vulkan surface is created
 *
 *		Global variables:
 *			window (extern)
 *			surface (extern)
 *
 *		Functions:
 *			createSurface()
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern GLFWwindow *window;
extern VkSurfaceKHR surface;
static VkResult createSurface() {
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
 * 	A physical device is chosen.
 *
 *	Global variables:
 *		requiredDeviceExtensions[requiredDeviceExtensionsCount + 1]
 *		//requiredQueueCommandsQueueFamilyIndex
 * 		physicalDevice (extern)
 *
 *	Functions:
 *		isDeviceSuitable()
 *		pickPhysicalDevice()
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
static constexpr uint32_t __requiredDeviceExtensionsCount = 1;
static uint32_t requiredDeviceExtensionsCount = __requiredDeviceExtensionsCount;
static const char *requiredDeviceExtensions[__requiredDeviceExtensionsCount + 1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};
static VkBool32 isPortabilitySubsetRequired = VK_FALSE;
//static struct {uint32_t graphicsFamily; uint32_t presentFamily;} requiredQueueCommandsQueueFamilyIndex;
static VkBool32 isDeviceSuitable(VkPhysicalDevice currPhysicalDevice) {
	// check if device supports api version 1.3 or higher
	VkPhysicalDeviceProperties2 deviceProperies;
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(currPhysicalDevice, &deviceProperies);
	VkBool32 isApiVersionSupported = deviceProperies.properties.apiVersion >= VK_API_VERSION_1_3;

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

	vkGetPhysicalDeviceFeatures2(currPhysicalDevice, &deviceFeatures);
	VkBool32 isAllFeaturesSupported = deviceVulkan13Features.dynamicRendering 
		&& deviceVulkanExtendedDynamicStateFeaturesEXT.extendedDynamicState;

	// check if requireds extensions are supported
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
	for(uint32_t i = 0; i < __requiredDeviceExtensionsCount; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedDeviceExtensionPropertiesCount; j++) {
			if (!strcmp(requiredDeviceExtensions[i], supportedDeviceExtensionProperties[j].extensionName)) {
				found = VK_TRUE;
				break;
			}
			if (!strcmp(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, supportedDeviceExtensionProperties[j].extensionName)) {
				isPortabilitySubsetRequired = VK_TRUE;
				requiredDeviceExtensionsCount++;
			} 
		}
		if (!found) {
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

	return isApiVersionSupported && isAllQueueCommandsSupported && isAllFeaturesSupported && isAllDeviceExtensionsSupported;
}

extern VkPhysicalDevice physicalDevice;
static VkBool32 pickPhysicalDevice() {
	uint32_t physicalDevicesCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr);
	if (physicalDevicesCount == 0) {
		fprintf(stderr, "ERROR: Unable to find any GPU with Vulkan support\n");
		return VK_FALSE;
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
		if (isDeviceSuitable(currPhysicalDevice) && physicalDevice == VK_NULL_HANDLE) {
			physicalDevice = currPhysicalDevice;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		fprintf(stderr, "ERROR: Unable to find any suitable GPU");
		return VK_FALSE;
	}
	VkPhysicalDeviceProperties2 chosenDeviceProperies;
	chosenDeviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	chosenDeviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &chosenDeviceProperies);
	printf("Picked device %d: %s\n", chosenDeviceProperies.properties.deviceID, chosenDeviceProperies.properties.deviceName);
	puts("");
	return VK_TRUE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			LOGICAL DEVICE CREATION
 *
 * 	A logical device is created.
 *
 *	Global variables:
 *		device (extern)
 *		queue (extern)
 *
 *	Functions:
 *		createLogicalDevice()
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern VkDevice device;
extern VkQueue queue; 
static VkResult createLogicalDevice() {
	VkResult result = VK_SUCCESS;

	// Finding the queueFamilyIndex that supports graphics and presentation commands simultanteously
	VkPhysicalDeviceProperties2 deviceProperies;
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperies);

	uint32_t queueFamilyIndex = ~0U;
	VkBool32 queueFamilyFound = VK_FALSE;
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, nullptr);
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

	VkPhysicalDeviceFeatures2 deviceFeatures = {0};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &deviceVulkan13Features;

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
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	

extern VkSwapchainKHR swapChain;
extern VkImage *swapChainImages;
extern VkSurfaceFormatKHR swapChainSurfaceFormat;
extern VkExtent2D swapChainExtent;
VkResult createSwapChain() {
	// Choosing surface format
	VkResult result = VK_SUCCESS;
	uint32_t surfaceFormatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr);
	VkSurfaceFormatKHR *surfaceFormats = malloc(sizeof(VkSurfaceFormatKHR)*surfaceFormatsCount);
	if (!surfaceFormats) {
		fprintf(stderr, "ERROR: Failed allocating memory for querying surface format\n");
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
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
	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);
	VkPresentModeKHR *presentModes = malloc(sizeof(VkPresentModeKHR)*presentModesCount);
	if (!presentModes) {
		fprintf(stderr, "ERROR: Failed allocating memory for querying present modes\n");
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}
	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, presentModes)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetPhysicalDeviceSurfacePresentModesKHR exited with error code %d\n", result);
		return result;
	}
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < presentModesCount; i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModes[i];
			break;
		}
	}
	free(presentModes);
	presentModes = nullptr;

	// Choosing swap extent
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VkExtent2D surfaceExtent = surfaceCapabilities.currentExtent;
	if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetPhysicalDeviceSurfaceCapabilitiesKHR exited with error code %d\n", result);
		return result;
	}
	if (surfaceCapabilities.currentExtent.width < ~0U && surfaceCapabilities.currentExtent.width > 0U) {
		int windowWidth, windowHeight;
		glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
		uint32_t w = surfaceCapabilities.currentExtent.width;
		w = w > surfaceCapabilities.maxImageExtent.width ? surfaceCapabilities.maxImageExtent.width : w;
		w = w < surfaceCapabilities.minImageExtent.width ? surfaceCapabilities.minImageExtent.width : w;
		surfaceExtent.width = w;
		uint32_t h = surfaceCapabilities.currentExtent.height;
		h = h > surfaceCapabilities.maxImageExtent.height ? surfaceCapabilities.maxImageExtent.height : h;
		h = h < surfaceCapabilities.minImageExtent.height ? surfaceCapabilities.minImageExtent.height : h;
		surfaceExtent.height = h;
	}
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
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = nullptr;

	if ((result = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreateSwapchainKHR exited with error code %d\n", result);
		return result;
	}

	uint32_t swapChainImagesCount;
	vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, nullptr);
	swapChainImages = malloc(sizeof(VkImage)*swapChainImagesCount);
	if (!swapChainImages) {
		fprintf(stderr, "ERROR: Failed allocating memory for swap chain images.\n");
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	if ((result = vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, swapChainImages)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkGetSwapchainImagesKHR exited with error code %d\n", result);
		return result;
	}
	swapChainSurfaceFormat = surfaceFormat;
	swapChainExtent = surfaceExtent;

	return result;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

VkResult initVulkan() {
	VkResult result = VK_SUCCESS;
	if ((result = createInstance()) < VK_SUCCESS) return result;
	if ((result = createSurface()) < VK_SUCCESS) return result;
	if ((result = pickPhysicalDevice()) < VK_SUCCESS) return result;
	if ((result = createLogicalDevice()) < VK_SUCCESS) return result;
	if ((result = createSwapChain()) < VK_SUCCESS) return result;
	return result;
}
