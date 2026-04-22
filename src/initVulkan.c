/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			VULKAN INTIALIZATION
 *
 * 		Baisc vulkan boilerplate: instance and device creation.
 *
 * 		The following external variables are initialized here:
 * 			instance
 * 			physicalDevice
 * 			device
 * 			graphicsQueue
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
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
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
		bool found = false;
		for (uint32_t j = 0; j < supportedLayersNum; j++) {
			if (strcmp(validationLayers[i], supportedLayers[j].layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Validation layer %s is not supported\n", validationLayers[i]);
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
		bool found = false;
		for (uint32_t j = 0; j < supportedExtensionsNum; j++) {
			if (strcmp(requiredInstanceExtensions[i], supportedExtensions[j].extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Required extension %s is not supported\n", requiredInstanceExtensions[i]);
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
		fprintf(stderr, "Failed allocating memory for extensions\n");
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
		fprintf(stderr, "vkCreateInstance failed with exit code: %d\n", result);
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
 *			PHYSICAL DEVICE CHOICE
 *
 * 	A physical device is chosen.
 *
 *	Global variables:
 *		requiredDeviceExtensions[requiredDeviceExtensionsCount + 1]
 *		requiredQueueCommandsQueueFamilyIndex
 * 		physicalDevice (extern)
 *
 *	Functions:
 *		isDeviceSuitable()
 *		pickPhysicalDevice()
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
constexpr uint32_t requiredDeviceExtensionsCount = 1;
static const char *requiredDeviceExtensions[requiredDeviceExtensionsCount + 1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};
static bool isPortabilitySubsetRequired = false;
static struct {uint32_t graphicsFamily;} requiredQueueCommandsQueueFamilyIndex;
static bool isDeviceSuitable(VkPhysicalDevice device) {
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
	if (!supportedDeviceExtensionProperties) {
		fprintf(stderr, 
			"Failed allocating memory for finding supported extensions of device: %d\n",
			deviceProperies.properties.deviceID);
		return false;
	}
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
	if (!queueFamiliesProperties) {
		fprintf(stderr, 
			"Failed allocating memory for finding queue families of device: %d\n",
			deviceProperies.properties.deviceID);
		return false;
	}
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
			requiredQueueCommandsQueueFamilyIndex.graphicsFamily = i;
			break;
		}
	}
	free(queueFamiliesProperties);
	queueFamiliesProperties = nullptr;

	return isApiVersionSupported && existsGraphicsFamily && isAllFeaturesSupported && isAllDeviceExtensionsSupported;
}

extern VkPhysicalDevice physicalDevice;
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
 *		device (extern)
 *
 *	Functions:
 *		createLogicalDevice()
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern VkDevice device;
static VkResult createLogicalDevice() {
	// Specifying queues
	float queuePriority = 0.5f;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {0};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.queueFamilyIndex = requiredQueueCommandsQueueFamilyIndex.graphicsFamily;
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

	VkResult result = VK_SUCCESS;

	if ((result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) < VK_SUCCESS) {
		fprintf(stderr, "vkCreateDevice failed with exit code: %d\n", result);
		return result;
	}

	return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *			QUEUE COMMANDS HANDLING
 *
 * 	Handles are created for required queue commands.
 *
 *	Global variables:
 *		graphicsQueue (extern)
 *
 *	Functions:
 *		initQueueHandles()
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	
extern VkQueue graphicsQueue; 
static VkResult initQueueHandles() {
	VkResult result = VK_SUCCESS;
 	vkGetDeviceQueue(device, requiredQueueCommandsQueueFamilyIndex.graphicsFamily, 0, &graphicsQueue);
	return result;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

VkResult initVulkan() {
	VkResult result = VK_SUCCESS;
	if ((result = createInstance()) < VK_SUCCESS) return result;
	if ((result = pickPhysicalDevice()) < VK_SUCCESS) return result;
	if ((result = createLogicalDevice()) < VK_SUCCESS) return result;
	if ((result = initQueueHandles()) < VK_SUCCESS) return result;
	return result;
}
