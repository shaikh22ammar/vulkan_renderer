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

extern const bool enableValidationLayers;
extern const bool usePortability;

static constexpr uint32_t validationLayersCount = 1;
static const char *validationLayers[validationLayersCount] = {"VK_LAYER_KHRONOS_validation"};
static RendererResult checkValidationLayerSupport() {
	/* Checks if the validation layers in the 
	 * global validationLayers is supported */
	/* Returns the index of the layer not present */

	uint32_t supportedLayersNum = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&supportedLayersNum, nullptr));
	VkLayerProperties supportedLayers[supportedLayersNum];
	VK_CHECK(vkEnumerateInstanceLayerProperties(&supportedLayersNum, supportedLayers));

	uint32_t i;
	for (i = 0; i < validationLayersCount; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedLayersNum; j++) {
			if (strcmp(validationLayers[i], supportedLayers[j].layerName) == 0) {
				found = VK_TRUE;
				break;
			}
		}
		if (!found) {
			break;
		}
	}
	if (i != validationLayersCount) {
		RR_SET_ERROR(RENDERER_ERR_INCOMPATIBILITY, 0, "validation layer %s not supported", validationLayers[i]);
		return RENDERER_ERR_INCOMPATIBILITY;
	}
	return RENDERER_SUCCESS;
}

static uint32_t requiredInstanceExtensionsCount = 0;
static const char **requiredInstanceExtensions = nullptr;
static RendererResult checkExtensionSupport() {
	/* Checks if the required instance extensions are supported in the instance 
	 * Exits if not supported */
	/* Returns the index of the extension not present */

	uint32_t supportedExtensionsNum = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, nullptr));
	VkExtensionProperties supportedExtensions[supportedExtensionsNum];

	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsNum, supportedExtensions));
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

	if (i != requiredInstanceExtensionsCount) {
		RR_SET_ERROR(RENDERER_ERR_INCOMPATIBILITY, 0, "extension %s not supported", requiredInstanceExtensions[i]);
		return RENDERER_ERR_INCOMPATIBILITY;
	}
	return RENDERER_SUCCESS;
}

extern VkInstance instance;
static RendererResult createInstance() {
	/* Creates an Instance
	 * First, checks if the given validation layers are supported (provided NDEBUG is not defined),
	 * Then checks for the required instance extensions (these are glfw extensions,
	 * and portability extensions for MacOS) */
	if (enableValidationLayers)
		RR_TRY(checkValidationLayerSupport());

	// Finding required extensions for glfw
	uint32_t glfwExtensionsCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	requiredInstanceExtensionsCount = glfwExtensionsCount;
	if (enableValidationLayers) requiredInstanceExtensionsCount++;
	if (usePortability) requiredInstanceExtensionsCount++;

	requiredInstanceExtensions = malloc((sizeof(const char *)) * requiredInstanceExtensionsCount);
	MALLOC_CHECK(requiredInstanceExtensionsCount);

	for (uint32_t i = 0; i < glfwExtensionsCount; i++) {
		requiredInstanceExtensions[i] = glfwExtensions[i];
	}

	/* The last element is populated contains the debug extension if required, 
	 * the next last element contains the portability extension if required */
	if (usePortability && !enableValidationLayers)
		requiredInstanceExtensions[requiredInstanceExtensionsCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	else if (usePortability && enableValidationLayers)
		requiredInstanceExtensions[requiredInstanceExtensionsCount - 2] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	if (enableValidationLayers)
		requiredInstanceExtensions[requiredInstanceExtensionsCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	puts("The following extensions are required:");
	for (uint32_t i = 0; i < requiredInstanceExtensionsCount; i++) {
		printf("%s, ", requiredInstanceExtensions[i]);
	}
	puts("");
	
	// Checking for extension support	
	RR_TRY(checkExtensionSupport());




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
	if(usePortability)
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	VkResult vk = vkCreateInstance(&createInfo, nullptr, &instance);
	RendererResult result = VK_WRITE(vk);

	free(requiredInstanceExtensions);
	requiredInstanceExtensions = nullptr;
	return result;
}







extern GLFWwindow *window;
extern VkSurfaceKHR surface;
static RendererResult createSurface() {
	/* The window system Vulkan is abstracted away by a VkSurface. */
	VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	return RENDERER_SUCCESS;
}










/* We always put portability extensions at the end of the array
 * requiredDeviceExtensions, but we change the size of the array
 * according to whether it is actually required */

// The compile-time constant number of device extensions required
static constexpr uint32_t __requiredDeviceExtensionsCount = 1;

/* the dependent number of device extensions required,
 * this will increase if portability extension is required 
 * for the chosen device */
static uint32_t requiredDeviceExtensionsCount = __requiredDeviceExtensionsCount;
static const char *requiredDeviceExtensions[__requiredDeviceExtensionsCount + 1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};
static uint32_t isDeviceSuitable(VkPhysicalDevice currPhysicalDevice) {
	/* Checks if currPhysicalDevice is suitable.
	 * A device is considered suitable if:
	 * It supports at least API version 1.3,
	 * It supports dynamic rendering and dynamic viewport state,
	 * It supports the required device extensions,
	 * If it has a queue family that supports both graphics and present command. */
	
	uint32_t result = 0U;
	VkExtensionProperties *supportedDeviceExtensionProperties = nullptr;
	VkQueueFamilyProperties2 *queueFamiliesProperties = nullptr;
	[[maybe_unused]] RendererResult _res;




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
	VkBool32 isAllFeaturesSupported = 
		deviceVulkan11Features.shaderDrawParameters 
		&& deviceVulkan13Features.dynamicRendering 
		&& deviceVulkan13Features.synchronization2
		&& deviceVulkanExtendedDynamicStateFeaturesEXT.extendedDynamicState;

	// check if requireds extensions are supported
	VkBool32 isPortabilitySubsetRequired = VK_FALSE;
	VkBool32 isAllDeviceExtensionsSupported = VK_TRUE;
	uint32_t supportedDeviceExtensionPropertiesCount;

	vkEnumerateDeviceExtensionProperties(currPhysicalDevice, nullptr, &supportedDeviceExtensionPropertiesCount, nullptr);
	supportedDeviceExtensionProperties 
		= malloc(sizeof(VkExtensionProperties)*supportedDeviceExtensionPropertiesCount);
	MALLOC_CHECK(supportedDeviceExtensionProperties);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(
			currPhysicalDevice, 
			nullptr, 
			&supportedDeviceExtensionPropertiesCount, 
			supportedDeviceExtensionProperties
	), _res, cleanup);
	for(uint32_t i = 0; i < __requiredDeviceExtensionsCount + 1; i++) {
		VkBool32 found = VK_FALSE;
		for (uint32_t j = 0; j < supportedDeviceExtensionPropertiesCount; j++) {
			if (!strcmp(requiredDeviceExtensions[i], supportedDeviceExtensionProperties[j].extensionName)) {
				found = VK_TRUE;
				if (i == __requiredDeviceExtensionsCount) {
					// Portability device extension found, hence required
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
	queueFamiliesProperties = malloc(sizeof(VkQueueFamilyProperties2)*queueFamiliesCount);
	MALLOC_CHECK(queueFamiliesProperties, _res, cleanup);

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
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(currPhysicalDevice, i, surface, &existsPresentFamily), _res, cleanup);
		isAllQueueCommandsSupported = existsGraphicsFamily && existsPresentFamily;
	}

	result |= isApiVersionSupported;
	result |= isAllQueueCommandsSupported << 1;
	result |= isAllFeaturesSupported << 2;
	result |= isAllDeviceExtensionsSupported << 3;
	result |= isPortabilitySubsetRequired << 4;

cleanup:
	if (supportedDeviceExtensionProperties)
		free(supportedDeviceExtensionProperties);
	if (queueFamiliesProperties) 
		free(queueFamiliesProperties);

	return result;
}

extern VkPhysicalDevice physicalDevice;
static RendererResult pickPhysicalDevice() {
	uint32_t physicalDevicesCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));
	if (physicalDevicesCount == 0) {
		RR_SET_ERROR(RENDERER_ERR_INCOMPATIBILITY, 0, "no devices found");
		return RENDERER_ERR_INCOMPATIBILITY;
	}
	physicalDevicesCount = physicalDevicesCount % 100; // in case stack overflows
	VkPhysicalDevice availablePhysicalDevices[physicalDevicesCount];
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, availablePhysicalDevices));

	puts("Found devices:");
	for (uint32_t i = 0; i < physicalDevicesCount; i++) {
		VkPhysicalDevice currPhysicalDevice = availablePhysicalDevices[i];
		VkPhysicalDeviceProperties2 currPhysicalDeviceProperies;
		currPhysicalDeviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		currPhysicalDeviceProperies.pNext = nullptr;
		vkGetPhysicalDeviceProperties2(currPhysicalDevice, &currPhysicalDeviceProperies);
		printf("%d: %s\n", currPhysicalDeviceProperies.properties.deviceID, currPhysicalDeviceProperies.properties.deviceName);		
		uint32_t res = isDeviceSuitable(currPhysicalDevice);
		if (res >= 0x1F && physicalDevice == VK_NULL_HANDLE) {
			physicalDevice = currPhysicalDevice;
			if (res | 1 << 4) 
				requiredDeviceExtensionsCount++;
			// portability support is required
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		RR_SET_ERROR(RENDERER_ERR_INCOMPATIBILITY, 0, "no supported devices found");
		return RENDERER_ERR_INCOMPATIBILITY;
	}

	VkPhysicalDeviceProperties2 chosenDeviceProperies;
	chosenDeviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	chosenDeviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &chosenDeviceProperies);
	printf("Picked device %d: %s\n", chosenDeviceProperies.properties.deviceID, chosenDeviceProperies.properties.deviceName);
	puts("");

	return RENDERER_SUCCESS;
}

extern VkDevice device;
extern VkQueue queue; 
extern uint32_t queueFamilyIndex;
static RendererResult createLogicalDevice() {
	/* Creates the logical device.
	 * Finds the queue family that supports both graphics and present command, initializes queue with that
	 * Note, if device requires portability extension, requiredDeviceExtensionsCount would already be incremented by now
	 * to reflect this */

	// Finding the queueFamilyIndex that supports graphics and presentation commands simultanteously
	VkPhysicalDeviceProperties2 deviceProperies;
	deviceProperies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperies.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperies);

	VkBool32 queueFamilyFound = VK_FALSE;
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, nullptr);
	VkQueueFamilyProperties2 *queueFamiliesProperties = malloc(sizeof(VkQueueFamilyProperties2)*queueFamiliesCount);
	MALLOC_CHECK(queueFamiliesProperties);

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
		RR_SET_ERROR(RENDERER_ERR_INCOMPATIBILITY, 0, "no queue family that supports both graphics and present commands");
		return RENDERER_ERR_INCOMPATIBILITY;
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

	VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
 	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

	return RENDERER_SUCCESS;
}


extern VkSwapchainKHR swapChain;
extern VkImage *swapChainImages;
extern uint32_t swapChainImagesCount;
extern VkSurfaceFormatKHR swapChainSurfaceFormat;
extern VkExtent2D swapChainExtent;
RendererResult createSwapChain() {
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
	/* We need to choose the format of the surface: colorspace and bit depth format */


	VkSurfaceFormatKHR *surfaceFormats = nullptr;
	VkPresentModeKHR *pPresentModes = nullptr;

	RendererResult result = RENDERER_SUCCESS;


	uint32_t surfaceFormatsCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr));

	surfaceFormats = malloc(sizeof(VkSurfaceFormatKHR)*surfaceFormatsCount);
	MALLOC_CHECK(surfaceFormats);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceFormats), result, cleanup);

	VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
	for (uint32_t i = 0; i < surfaceFormatsCount; i++) {
		if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = surfaceFormats[i];
			break;
		}
	}
	free(surfaceFormats);
	surfaceFormats = nullptr;

	// Choosing present modes
	/* Present modes state what should be done when application and monitor refresh rate is out of sync */
	uint32_t presentModesCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr), result, cleanup);
	pPresentModes = malloc(sizeof(VkPresentModeKHR)*presentModesCount);
	MALLOC_CHECK(pPresentModes, result, cleanup);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, pPresentModes), result, cleanup);

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
#ifdef NDEBUG
	for (uint32_t i = 0; i < presentModesCount; i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModes[i];
			break;
		}
	}
#endif

	// Choosing swap extent
	/* We need to state the extent of the surface (dimension) 
	 * By default, it would be the extent of the window,
	 * but if the width is ~0U, it needs to be specified */
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities), result, cleanup);
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

	VK_CHECK(vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain), result, cleanup);

	// Creating swap chain images array
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, nullptr), result, cleanup);
	swapChainImages = malloc(sizeof(VkImage)*swapChainImagesCount);
	MALLOC_CHECK(swapChainImages, result, cleanup);
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, swapChainImages), result, cleanup);
	swapChainSurfaceFormat = surfaceFormat;
	swapChainExtent = surfaceExtent;

cleanup:
	if (surfaceFormats) free(surfaceFormats);
	if (pPresentModes) free(pPresentModes);

	return result;
}


extern VkImageView *swapChainImageViews;
RendererResult createImageViews() {
	/* Image view determine how to actually interpret the image
	 * irregardless of its format. For our purpose of swapchains,
	 * we want interpretation to be same as the format */
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
		VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, swapChainImageViews + i));
	}
	return RENDERER_SUCCESS;
}


/* Debug */

#ifndef NDEBUG
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
		VkInstance                                  instance,
		const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
		const VkAllocationCallbacks*                pAllocator,
		VkDebugUtilsMessengerEXT*                   pMessenger) {

	auto f = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (!f) return VK_ERROR_EXTENSION_NOT_PRESENT;
	return f(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
		VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator){
	auto f = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	return f(instance, messenger, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
		VkDevice device,
		const VkDebugUtilsObjectNameInfoEXT* pNameInfo
		) {
	auto f = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	if (!f) return VK_ERROR_EXTENSION_NOT_PRESENT;
	return f(device, pNameInfo);
}

RendererResult rrSetDebugObjectName (
		VkObjectType objectType,
		uint64_t objectHandle,
		const char *objectName
		) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = objectType,
		.objectHandle = objectHandle,
		.pObjectName = objectName
	};
	VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &nameInfo));
	return RENDERER_SUCCESS;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	fprintf(pUserData, "validation layer: %s\n", pCallbackData->pMessage);

	return VK_FALSE;
}

extern VkDebugUtilsMessengerEXT debugMessenger;
static RendererResult setupDebugMessenger() {
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
	createInfo.pUserData = stderr;
	
	VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));

	return RENDERER_SUCCESS;
}

#endif

#include "utils/functionQueue.h"
extern struct functionStack cleanupFunctions;

static void destroyThings() {
	for (uint32_t i = 0; i < swapChainImagesCount; i++) {
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	swapChainImageViews = nullptr;

	// swapchain
	vkDestroySwapchainKHR(device, swapChain, nullptr);
	swapChainImages = nullptr;

	// device
	vkDestroyDevice(device, nullptr);
	device = nullptr;

	// surface
	vkDestroySurfaceKHR(instance, surface, nullptr);
	surface = nullptr;

	// instance
#ifndef NDEBUG
	vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
	vkDestroyInstance(instance, nullptr);
	instance = nullptr;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

RendererResult initVulkan() {
	RR_TRY(createInstance());
#ifndef NDEBUG
	RR_TRY(setupDebugMessenger());
#endif
	RR_TRY(createSurface());
	RR_TRY(pickPhysicalDevice()); 
	RR_TRY(createLogicalDevice()); 
	RR_TRY(createSwapChain()); 
	RR_TRY(createImageViews());
	functionStack_insert(&cleanupFunctions, destroyThings);
	return RENDERER_SUCCESS;
}
