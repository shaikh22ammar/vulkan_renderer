#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "constants.h"
#include "rendererErrors.h"
#include <cglm/cglm.h>
#include "types.h"

#include <math.h>
#include <time.h>
#include "utils/matrices.h"

extern unsigned int currentFrameInFlight;

extern VkQueue queue; 
extern VkDevice device;
extern VkCommandBuffer *pCommandBuffers;

extern VkSwapchainKHR swapChain;
extern VkImage *swapChainImages;
extern VkImageView *swapChainImageViews;
extern VkExtent2D swapChainExtent;

extern VkPipeline graphicsPipeline;
extern VkPipelineLayout pipelineLayout;

extern VkDeviceSize indexOffset;
extern VkDeviceSize vertexBufferSize;
extern VkBuffer vertexBuffer;
extern int numVertices;
extern int numIndices;

extern VkDescriptorSet descriptorSet;

extern VkSemaphore *pAcquiredImageSemaphores;
extern VkFence *pDrawingDoneFences;
extern VkSemaphore *pRenderingDoneSemaphores;



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

void updatePushConstants(struct pushConstants *pc) {

	static struct timespec startTime;
	static bool initialized = false;
	if (!initialized) {
		clock_gettime(CLOCK_MONOTONIC, &startTime);
		initialized = true;
	}

	struct timespec currentTime;
	clock_gettime(CLOCK_MONOTONIC, &currentTime);

	double elapsed = (currentTime.tv_sec  - startTime.tv_sec) +
		    (currentTime.tv_nsec - startTime.tv_nsec) / 1e9;

	float time = (float) fmod(elapsed, 360.0 / 90.0);

	mat4 model;
	vec3 down = {0.0f, 1.0f, 0.0f};
	glm_mat4_identity(model);
	glm_rotate_x(model, glm_rad(-90), model);
	glm_rotate_z(model, glm_rad(90)*time, model);

	// view the 2d object from above and behind
	vec3 eye = {0.0f, -2.0f, -2.0f};
	vec3 center = {0, 0, 0};
	mat4 view;
	glm_lookat_vk(eye, center, down, view);

	mat4 persp;
	glm_perspective_vk(45.0f, (float) swapChainExtent.width / swapChainExtent.height, 0.01f, 10.0f, persp);

	glm_mul(view, model, pc->mvp);
	glm_mat4_mul(persp, pc->mvp, pc->mvp);
}

static RendererResult recordCommandBuffer(uint32_t imageIndex) {
	VkCommandBuffer commandBuffer = pCommandBuffers[currentFrameInFlight];
	vkResetCommandBuffer(commandBuffer, 0);
	VkCommandBufferBeginInfo commandBufferBeginInfo = {0};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
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
	vkCmdBindIndexBuffer(commandBuffer, vertexBuffer, indexOffset, VK_INDEX_TYPE_UINT16);
	
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	struct pushConstants pc;
	updatePushConstants(&pc);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &pc);
	//vkCmdPushConstants2(commandBuffer, &pushConstantsInfo);

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
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	return RENDERER_SUCCESS;
}

RendererResult drawFrame() {
	/* Before recording into the command buffer 
	 * of the current FIF, make sure it has been executed */
	VK_CHECK(vkWaitForFences(device, 1, pDrawingDoneFences + currentFrameInFlight, VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(device, 1, pDrawingDoneFences + currentFrameInFlight));

	/* Acquire the next image in the swapchain to be presented.
	 * Signal when image is acquired */
	uint32_t nextImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, pAcquiredImageSemaphores[currentFrameInFlight], VK_NULL_HANDLE, &nextImageIndex));


	/* Start recording the command buffer for the current FIF 
	 * with the next image in swapchain as attachment */
	recordCommandBuffer(nextImageIndex);

	VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = pCommandBuffers[currentFrameInFlight],
		.deviceMask = 0
	};

	/* Do not start executing the command buffer commands
	 * for color attachment until the next image
	 * has been acquired */
	VkSemaphoreSubmitInfo waitForImageAcquisitionSemaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = pAcquiredImageSemaphores[currentFrameInFlight],
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.deviceIndex = 0
	};

	/* Signal the render complete semaphore for the current image
	 * after all graphics commands in the buffer (and all the commands 
	 * submitted prior to the queue) is done */
	VkSemaphoreSubmitInfo signalRenderCompleteSemaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = pRenderingDoneSemaphores[nextImageIndex],
		.stageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		.deviceIndex = 0
	};

	VkSubmitInfo2 submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &waitForImageAcquisitionSemaphoreInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commandBufferSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signalRenderCompleteSemaphoreInfo
	};

	/* Submit the drawing commands.
	 * Signal the drawFence when the commands finish executing */
	VK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, pDrawingDoneFences[currentFrameInFlight]));

	/* Wait for the rendering to be done for the current FIF, 
	 * to the next swapchain image. Present the render to the next image */
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = pRenderingDoneSemaphores + nextImageIndex,
		.swapchainCount = 1,
		.pSwapchains = &swapChain,
		.pImageIndices = &nextImageIndex,
		.pResults = nullptr
	};

	VkResult vr = vkQueuePresentKHR(queue, &presentInfo);
	if (vr == VK_SUBOPTIMAL_KHR || vr == VK_ERROR_OUT_OF_DATE_KHR) {
		//TODO update swapchain
		vr = VK_SUCCESS;
	}
	VK_CHECK(vr);

	currentFrameInFlight = (currentFrameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;

	return RENDERER_SUCCESS;
}
