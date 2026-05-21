#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "rendererErrors.h"

#include "constants.h"

extern VkDevice device;
extern uint32_t queueFamilyIndex;
extern VkCommandPool commandPool;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer *pCommandBuffers;
extern VkImage *swapChainImages;
extern VkImageView *swapChainImageViews;
extern VkExtent2D swapChainExtent;
extern VkPipeline graphicsPipeline;
extern VkBuffer vertexBuffer;
extern VkBuffer indexBuffer;
extern int numIndices;
extern int numVertices;

extern unsigned int currentFrameInFlight;

static VkResult createCommandPool() {
	/* Command pools are allocators for command buffers.
	 * Since allocating and freeing command buffers individually would be
	 * too expensive, implementations offer specialized allocators called
	 * command pools that can reuse memory */
	VkResult result;
	VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
					/* Command buffers in this pool can be
					 * reset individually withou having to 
					 * reset the entire pool */
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
	return result;
}


static VkResult createTransferCommandPool() {
	/* Command pools are allocators for command buffers.
	 * Since allocating and freeing command buffers individually would be
	 * too expensive, implementations offer specialized allocators called
	 * command pools that can reuse memory */
	VkResult result;
	VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
					/* Command buffers in this pool are short-lived
					 * */
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &transferCommandPool);
	return result;
}

static VkResult createCommandBuffer() {
	VkResult result;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	pCommandBuffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
	if (!pCommandBuffers) return VK_ERROR_OUT_OF_HOST_MEMORY;
	result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffers);

	return result;
}

static void transitionImageLayout(
		uint32_t imageIndex,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkAccessFlags2 srcAccesMask,
		VkAccessFlags2 dstAccesMask,
		VkPipelineStageFlags2 srcPipelineStageFlags2,
		VkPipelineStageFlags2 dstPipelineStageFlags2
		) {
	/* Images are laid out in specific formats 
	 * that are most optimal for the purpose.
	 * For example, Morton curves for rendering */

	/* This function records an image transition in
	 * the command buffer */
	VkCommandBuffer commandBuffer = pCommandBuffers[currentFrameInFlight];
	VkImageMemoryBarrier2 imageMemoryBarrier = {0};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imageMemoryBarrier.pNext = nullptr;
	imageMemoryBarrier.srcStageMask = srcPipelineStageFlags2;
	imageMemoryBarrier.srcAccessMask = srcAccesMask;
	imageMemoryBarrier.dstStageMask = dstPipelineStageFlags2;
	imageMemoryBarrier.dstAccessMask = dstAccesMask;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = swapChainImages[imageIndex];
	imageMemoryBarrier.subresourceRange = (VkImageSubresourceRange) {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	VkDependencyInfo dependencyInfo = {0};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.pNext = nullptr;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

VkResult recordCommandBuffer(uint32_t imageIndex) {
	VkCommandBuffer commandBuffer = pCommandBuffers[currentFrameInFlight];
	vkResetCommandBuffer(commandBuffer, 0);
	VkCommandBufferBeginInfo commandBufferBeginInfo = {0};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	handleVulkanError(result, "vkBeginCommandBuffer", true);
	/* Before recording, we need to transition the image into the
	 * optimal layout for color attachment.
	 * We need an image memory barrier for this.
	 * The src and dst stage should be output of color attachment output.
	 * The src access should be null, and dst access color attachment write.
	 * Old layout should be undefined and new layout should be color attachment optimal.
	 * This means that between two color attachent output, a memory barrier is put
	 * that transitions the image to optimal for color attachment discarding the previous image.
	 * I don't know why dst access is not null */

	transitionImageLayout(
			imageIndex,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_NONE,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkClearValue clearColor = {0};
	clearColor.color.float32[0] = 0.0f;
	clearColor.color.float32[1] = 0.0f;
	clearColor.color.float32[2] = 0.0f;
	clearColor.color.float32[3] = 1.0f;

	// Draw commands must be recorded within a render pass instance. 
	// Each render pass instance defines a set of image resources, referred to as attachments, used during rendering
	
	/* the rendering attachment is the swapchain image
	 * we need to specify certain things like should it be clear before rendering
	 * should it be stored after rendering
	 * what is its layout etc */
	VkRenderingAttachmentInfo renderingAttachmentInfo = {0};
	renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	renderingAttachmentInfo.pNext = nullptr;
	renderingAttachmentInfo.imageView = swapChainImageViews[imageIndex];
	renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	renderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	renderingAttachmentInfo.clearValue = clearColor;

	VkRenderingInfo renderingInfo = {0};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.renderArea.offset = (VkOffset2D) {0, 0};
	renderingInfo.renderArea.extent = swapChainExtent; 
	renderingInfo.layerCount = 1; 
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &renderingAttachmentInfo;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	/* commands need information on the state (as described in src/graphicsPipeline.c)
	 * so, we need to bind the command buffer to a pipeline */
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = swapChainExtent.width;
	viewport.height = swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D) {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkDeviceSize pZeros[1] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, pZeros);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	//vkCmdDraw(commandBuffer, numVertices, 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
	vkCmdEndRendering(commandBuffer);

	transitionImageLayout(
			imageIndex, 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_NONE,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
	/* When transitioning the image to the appropriate layout, 
	 * there is no need to delay subsequent processing, or perform any visibility operations 
	 * (as vkQueuePresentKHR performs automatic visibility operations). 
	 * To achieve this, the dstAccessMask member of the VklmageMemoryBarrier should be 0, 
	 * and the dstStageMask parameter should be VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT. */
	result = vkEndCommandBuffer(commandBuffer);
	handleVulkanError(result, "vkEndCommandBuffer", true);
	return result;
}

void initCommandBuffers() {
	constexpr int numFunctions = 3;
	VkResult (*functionsToCall[numFunctions])(void) = {
		createCommandPool,
		createTransferCommandPool,
		createCommandBuffer
	};
	const char *functionsToCallNames[numFunctions] = {
		"createCommandPool",
		"createTransferCommandPool",
		"createCommandBuffer"
	};
	for (int i = 0; i < numFunctions; i++) {
		handleVulkanError(functionsToCall[i](), functionsToCallNames[i], true);
	}
}
