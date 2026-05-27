#include "rendererErrors.h"
#include "utils/functionQueue.h"
#include "vulkan/vulkan_core.h"
//#include "constants.h"

extern VkDevice device;
extern VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;

extern VkImageView textureImageView;
extern VkSampler textureImageSampler;

static VkDescriptorPool descriptorPool;
static VkDescriptorSetLayout descriptorLayout;
extern VkDescriptorSet descriptorSet;

static RendererResult createDescriptors() {
// layout creation
	VkDescriptorSetLayoutBinding imageBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	VkDescriptorSetLayoutCreateInfo descLayout = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &imageBinding
	};
	VK_CHECK(vkCreateDescriptorSetLayout(device, &descLayout, nullptr, &descriptorLayout));
	pipelineLayoutCreateInfo.setLayoutCount++;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorLayout;

// descriptor allocation
	VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1
	};
	VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.pPoolSizes = &poolSize,
		.poolSizeCount = 1
	};
	VK_CHECK(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool));

	VkDescriptorSetAllocateInfo descSetAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorLayout
	};
	VK_CHECK(vkAllocateDescriptorSets(device, &descSetAllocInfo, &descriptorSet));
	
// descriptor binding
	VkDescriptorImageInfo descImageInfo = {
		.sampler = textureImageSampler,
		.imageView = textureImageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet descWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &descImageInfo
	};
	vkUpdateDescriptorSets(device, 1, &descWrite, 0, nullptr);
	return RENDERER_SUCCESS;
}

extern struct functionStack cleanupFunctions;
static void destroyDescriptors() {
	vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

#ifndef NDEBUG
RendererResult rrSetDebugObjectName (
		VkObjectType objectType,
		uint64_t objectHandle,
		const char *objectName
		);
#endif

RendererResult initDescriptors() {
	RR_TRY(createDescriptors());
	functionStack_insert(&cleanupFunctions, destroyDescriptors);

#ifndef NDEBUG
	rrSetDebugObjectName (
			VK_OBJECT_TYPE_DESCRIPTOR_SET,
			(uint64_t) descriptorSet,
			"descriptor set"
			);
	rrSetDebugObjectName (
			VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
			(uint64_t) descriptorLayout,
			"descriptor set layout"
			);
#endif
	return RENDERER_SUCCESS;
}
