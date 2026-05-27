#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <cglm/cglm.h>
#include "utils/functionQueue.h"
#include "types.h"


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "rendererErrors.h"




// error callback function, executes on every error encounter
static void onRendererError(const RendererErrorInfo *info, void *userData) {
	FILE *log = (FILE *)userData;

	fprintf(log, "[renderer] %s (%s:%d) code=%d raw=%d — %s\n",
		info->function,
		info->file,
		info->line,
		info->result,
		info->rawCode,
		info->message);
}
const RendererErrorInfo *rendererGetLastError(void) {
	return &gLastError;
}
thread_local RendererErrorInfo gLastError;
pFnRendererErrorCallback gErrorCb;
void *gErrorCbUserdata;





#ifdef VALIDATION 
const bool enableValidationLayers = true;
#else 
const bool enableValidationLayers = false;
#endif
#ifdef PORTABILITY
const bool usePortability = true;
#else
const bool usePortability = false;
#endif




unsigned int currentFrameInFlight = 0;
uint32_t WIDTH 	= 800;
uint32_t HEIGHT = 600;




/* All global variables with external linkage are defined here. 
 * Every other global variable has internal linkage.
 * Above every variable, the translation unit where they are 
 * "truly" defined is written */

// initWindow
GLFWwindow *window = nullptr;

// initVulkan
VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkPhysicalDeviceProperties2 physicalDeviceProperties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
VkDevice device = VK_NULL_HANDLE;
VkQueue queue = VK_NULL_HANDLE; 
uint32_t queueFamilyIndex = 0;
VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

VkSwapchainKHR swapChain = VK_NULL_HANDLE;
VkImage *swapChainImages = VK_NULL_HANDLE;
uint32_t swapChainImagesCount = 0;
VkSurfaceFormatKHR swapChainSurfaceFormat = {0};
VkExtent2D swapChainExtent = {0};
VkImageView *swapChainImageViews = VK_NULL_HANDLE;

// graphicsPipeline vertexBuffer pushConstants
VkPipeline graphicsPipeline = VK_NULL_HANDLE;
VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {0};
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {0};

// commandBuffers
VkCommandPool commandPool = VK_NULL_HANDLE;
VkCommandPool transferCommandPool = VK_NULL_HANDLE;
VkCommandBuffer transferCommandBuffer = VK_NULL_HANDLE;
VkCommandBuffer *pCommandBuffers = nullptr;

// vertexBuffers
VkDeviceSize indexOffset = 0;
VkDeviceSize vertexBufferSize = 0;
VkBuffer vertexBuffer = VK_NULL_HANDLE;
int numVertices = 0;
int numIndices = 0;

// syncObjects
VkSemaphore *pAcquiredImageSemaphores = nullptr;
VkFence *pDrawingDoneFences = nullptr;
VkSemaphore *pRenderingDoneSemaphores = nullptr;

// image
VkImageView textureImageView = VK_NULL_HANDLE; 
VkSampler textureImageSampler = VK_NULL_HANDLE;

// descriptors
VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

// pushConstants
struct pushConstants pushConstants = {0};
/*--------------------------------------------------------------------------------*/

extern void drawFrame();
static void mainLoop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(device);
}

struct functionStack cleanupFunctions = {0};
static void cleanUp() {
	functionStack_call(&cleanupFunctions);
}

extern RendererResult initWindow();
extern RendererResult initVulkan();
extern RendererResult initGraphicsPipelineCreateInfo();
extern RendererResult initCommandBuffers();
extern RendererResult initSyncObjects();
extern RendererResult initShaders();
extern RendererResult initVertices();
extern RendererResult initTexture();
extern RendererResult initDescriptors();
extern RendererResult initPushConstants();
extern RendererResult createGraphicsPipeline();
static RendererResult run() {
	RendererResult result;

	RR_TRY(initWindow(), result, cleanup);
	RR_TRY(initVulkan(), result, cleanup);
	RR_TRY(initGraphicsPipelineCreateInfo(), result, cleanup);
	RR_TRY(initCommandBuffers(), result, cleanup);
	RR_TRY(initSyncObjects(), result, cleanup);
	RR_TRY(initShaders(), result, cleanup);
	RR_TRY(initVertices(), result, cleanup);
	RR_TRY(initTexture(), result, cleanup);
	RR_TRY(initDescriptors(), result, cleanup);
	RR_TRY(initPushConstants(), result, cleanup);
	RR_TRY(createGraphicsPipeline(), result, cleanup);

	mainLoop();

cleanup:
	cleanUp();
	return result;
}


int main() {
	gErrorCb = onRendererError;
	gErrorCbUserdata = stderr;
	gLastError = (RendererErrorInfo) {0}; 

	run();

	return 0;
}
