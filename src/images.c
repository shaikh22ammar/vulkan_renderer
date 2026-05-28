#include "rendererErrors.h"
#include "utils/functionQueue.h"
#include "vulkan/vulkan_core.h"
#define STB_IMAGE_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall" 
#pragma clang diagnostic ignored "-Wextra" 
#pragma clang diagnostic ignored "-Wdouble-promotion" 
#pragma clang diagnostic ignored "-Wfloat-conversion"
#include <stb_image.h>
#pragma clang diagnostic pop

extern VkDevice device;
extern VkDevice physicalDevice;
extern VkPhysicalDeviceProperties2 physicalDeviceProperties;
extern uint32_t findMemoryTypes(uint32_t supportedMemoryTypes, VkMemoryPropertyFlags requiredMemoryProperties);
extern VkQueue queue;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer transferCommandBuffer;
extern VkExtent2D swapChainExtent;

static VkBuffer stagingBuffer;
static VkDeviceMemory stagingBufferMemory;

static VkImage textureImage;
extern VkImageView textureImageView;
extern VkSampler textureImageSampler;
static VkDeviceMemory textureImageMemory;

extern VkImage depthImage;
extern VkImageView depthView;
static VkDeviceMemory depthImageMemory;



static RendererResult transitionImageLayoutInferStage(
		VkImage image, 
		VkImageLayout oldLayout, VkImageLayout newLayout, 
		VkCommandBuffer commandBuffer) {

	VkImageMemoryBarrier2 imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
			.levelCount = 1, 
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		imageMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		imageMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	} else {
		RR_SET_ERROR(RENDERER_ERR_INVALID_ARG, 0, "unsupported layer transition");
		return RENDERER_ERR_INVALID_ARG;
	}

	VkDependencyInfo dependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = nullptr,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageMemoryBarrier
	};

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	return RENDERER_SUCCESS;
}

static RendererResult createTextureImage() {
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load(ASSET_DIR "/textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels) {
		RR_SET_ERROR(RENDERER_ERR_IMAGE, 0, "textures/texture.jpg could not be loaded");
		return RENDERER_ERR_IMAGE;
	}
/* staging buffer */
	VkBufferCreateInfo stagingBuffCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.size = imageSize,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VK_CHECK(vkCreateBuffer(device, &stagingBuffCreateInfo, nullptr, &stagingBuffer));
	VkMemoryRequirements stagingBuffMemRequirements;
	vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBuffMemRequirements);
	VkMemoryAllocateInfo stagingBuffMemAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = stagingBuffMemRequirements.size,
		.memoryTypeIndex = 
			findMemoryTypes(
				stagingBuffMemRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				)
	};
	VK_CHECK(vkAllocateMemory(device, &stagingBuffMemAllocInfo, nullptr, &stagingBufferMemory));
	VK_CHECK(vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0));
	void *data;
	VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data));
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(device, stagingBufferMemory);
	stbi_image_free(pixels);

/* device local buffer */
	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D, 
		.format = VK_FORMAT_R8G8B8A8_SRGB, 
		.extent = {texWidth, texHeight, 1}, 
		.mipLevels = 1, 
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT, 
		.tiling = VK_IMAGE_TILING_OPTIMAL, 
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE};
	VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &textureImage));
	VkMemoryRequirements texMemReq;
	vkGetImageMemoryRequirements(device, textureImage, &texMemReq);
	VkMemoryAllocateInfo texMemAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = texMemReq.size,
		.memoryTypeIndex = findMemoryTypes(texMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_CHECK(vkAllocateMemory(device, &texMemAllocInfo, nullptr, &textureImageMemory));
	VK_CHECK(vkBindImageMemory(device, textureImage, textureImageMemory, 0));

/* copying staging buffer to vertex buffer */
	VkCommandBufferBeginInfo transferBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	VK_CHECK(vkBeginCommandBuffer(transferCommandBuffer, &transferBufferBeginInfo));
		RR_TRY(transitionImageLayoutInferStage(
					textureImage, 
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
					transferCommandBuffer));
		VkBufferImageCopy bufferImageCopy = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
				.mipLevel = 0, 
				.baseArrayLayer = 0, 
				.layerCount = 1
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = {texWidth, texHeight, 1}
		};
		vkCmdCopyBufferToImage(
				transferCommandBuffer, 
				stagingBuffer, textureImage, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				1, &bufferImageCopy);
		RR_TRY(transitionImageLayoutInferStage(
					textureImage, 
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
					transferCommandBuffer));
	VK_CHECK(vkEndCommandBuffer(transferCommandBuffer));

	VkCommandBufferSubmitInfo transferCommandBufferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = transferCommandBuffer,
		.deviceMask = 0
	};
	VkSubmitInfo2 transferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &transferCommandBufferSubmitInfo
	};
	VK_CHECK(vkQueueSubmit2(queue, 1, &transferSubmitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(queue));

	vkResetCommandPool(device, transferCommandPool, 0);

	return RENDERER_SUCCESS;
}

static RendererResult createDepthBuffer() {
	VkImageCreateInfo depthImageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_D32_SFLOAT,
		.extent = {.width = swapChainExtent.width, .height = swapChainExtent.height, .depth = 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = 1,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_CHECK(vkCreateImage(device, &depthImageCreateInfo, nullptr, &depthImage));
	VkMemoryRequirements depthMemReqs;
	vkGetImageMemoryRequirements(device, depthImage, &depthMemReqs);
	VkMemoryAllocateInfo memAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = depthMemReqs.size,
		.memoryTypeIndex = findMemoryTypes(depthMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr, &depthImageMemory));
	vkBindImageMemory(device, depthImage, depthImageMemory, 0);

	VkImageViewCreateInfo depthViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_D32_SFLOAT,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VK_CHECK(vkCreateImageView(device, &depthViewCreateInfo, nullptr, &depthView));
	return RENDERER_SUCCESS;
}

static RendererResult createTextureImageView() {
	VkImageViewCreateInfo imageViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.flags = 0,
		.image = textureImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &textureImageView));
	return RENDERER_SUCCESS;
}

static RendererResult createTextureImageSampler() {
	VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod	= 0.0f,
		.maxLod = 0.0f,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = physicalDeviceProperties.properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureImageSampler));

	return RENDERER_SUCCESS;
}

extern struct functionStack cleanupFunctions;
static void destroyImages() {
	vkDestroyImage(device, depthImage, nullptr);
	vkDestroyImageView(device, depthView, nullptr);
	vkDestroyImageView(device, textureImageView, nullptr);
	vkDestroyImage(device, textureImage, nullptr);
	vkDestroySampler(device, textureImageSampler, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);
	vkFreeMemory(device, textureImageMemory, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

#ifndef NDEBUG
RendererResult rrSetDebugObjectName (
		VkObjectType objectType,
		uint64_t objectHandle,
		const char *objectName
		);
#endif

RendererResult initTexture() {
	RR_TRY(createTextureImage());
	RR_TRY(createTextureImageView());
	RR_TRY(createTextureImageSampler());
	RR_TRY(createDepthBuffer());

	functionStack_insert(&cleanupFunctions, destroyImages);
#ifndef NDEBUG
	rrSetDebugObjectName (
			VK_OBJECT_TYPE_IMAGE_VIEW,
			(uint64_t) textureImageView,
			"texture image view"
			);
	rrSetDebugObjectName (
			VK_OBJECT_TYPE_SAMPLER,
			(uint64_t) textureImageSampler,
			"texture image sampler"
			);
#endif
	return RENDERER_SUCCESS;
}
