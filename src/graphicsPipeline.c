/* 
 * GPUs are essentially massive state machines.
 * How the commands that we pass behave depend on the state (a collection of variables, information, etc.).
 * Different "work" (rasterization, compute, raytracing, etc.) require different state variables.
 * These different "work" need different blueprint of variables to describe the state.
 * For example, a rasterization work would involve a variable describing the multisampling behaviour,
 * but that does not make sense in a compute work.
 * The different blueprints are called pipelines, not to be confused with pipeline stages.
 * We will only work with the rasterization/graphics pipeline,
 * for which we need to create a graphics pipeline explictly stating the state.
 * For viewport and scissor, we will specify a dynamic state meaning
 * that commands we pass will specify/modify the state 
 * */


#include <stdio.h>
#include <stdlib.h>
#include "readFile.h"
#include <vulkan/vulkan_core.h>
#include "rendererErrors.h"

#define SHADER_DIR "shaders/"

extern VkDevice device;
extern VkExtent2D swapChainExtent;
extern VkSurfaceFormatKHR swapChainSurfaceFormat;
extern VkPipelineLayout pipelineLayout;
extern VkPipeline graphicsPipeline;

VkResult createShaderModule(const uint32_t *shaderCode, const size_t shaderCodeSize, VkShaderModule *shaderModule) {
	// shader code needs to be 32 bit aligned
	if ((uint64_t) shaderCode % 4 != 0) {
		return VK_ERROR_INVALID_SHADER_NV;
	}

	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.codeSize = shaderCodeSize;
	createInfo.pCode = shaderCode;
	return vkCreateShaderModule(device, &createInfo, nullptr, shaderModule);
}

void initGraphicsPipeline() {
	char *filepath = SHADER_DIR "triangle.spv";
	uint32_t *shaderCode = nullptr;
	size_t shaderCodeSize = 0;
	ReadFileResult shaderCodeReadingResult;
	shaderCodeReadingResult = readFile(filepath, nullptr, &shaderCodeSize);
	handleReadFileError(shaderCodeReadingResult, true);
	shaderCode = malloc(sizeof(char)*shaderCodeSize);
	if (!shaderCode) {
		fprintf(stderr, "ERROR: Failed to allocated memory for shader code\n");
		exit(RENDERER_ERROR_OUT_OF_MEMORY);
	}
	shaderCodeReadingResult = readFile(filepath, (char *) shaderCode, &shaderCodeSize);
	handleReadFileError(shaderCodeReadingResult, true);

	VkShaderModule shaderModule;
	VkResult shaderModuleCreationResult;
	shaderModuleCreationResult = createShaderModule(shaderCode, shaderCodeSize, &shaderModule);
	handleVulkanError(shaderModuleCreationResult, "createShaderModule", true);
	free(shaderCode);
	shaderCode = nullptr;

	// Shader stage pipeline
	constexpr int shaderStagesCount = 2;

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";

	VkPipelineShaderStageCreateInfo shaderStages[shaderStagesCount] = {vertShaderStageInfo, fragShaderStageInfo};

	// Dynamic states
	// Viewport and scissors are set to be dynamic
	constexpr int dynamicStatesCount = 2;
	VkDynamicState dynamicStates[dynamicStatesCount] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo = {0};
	dynamicStatesCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStatesCreateInfo.pNext = nullptr;
	dynamicStatesCreateInfo.dynamicStateCount = dynamicStatesCount;
	dynamicStatesCreateInfo.pDynamicStates = dynamicStates;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {0};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;
	
	// vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {0};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.pNext = nullptr;

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {0};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.pNext = nullptr;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 
	// rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {0};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.pNext = nullptr;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.lineWidth = 1.0f;

	// multisample
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {0};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext = nullptr;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;

	// color blending
	// After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. 
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {0};
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = 
				 	  VK_COLOR_COMPONENT_R_BIT 
					| VK_COLOR_COMPONENT_G_BIT
					| VK_COLOR_COMPONENT_B_BIT
					| VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {0};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext = nullptr;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {0};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

	VkResult pipelineLayoutCreationResult;
	pipelineLayoutCreationResult = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout); 
	handleVulkanError(pipelineLayoutCreationResult, "vkCreatePipelineLayout", true);

	// create graphics pipeline
	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {0};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipelineRenderingCreateInfo.pNext = nullptr;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainSurfaceFormat.format;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {0};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					// This is needed since render pass is not used
	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
	graphicsPipelineCreateInfo.stageCount = shaderStagesCount;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStatesCreateInfo;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = nullptr;

	VkResult graphicsPipelineCreationResult;
	graphicsPipelineCreationResult = vkCreateGraphicsPipelines(
			device, 
			VK_NULL_HANDLE, // pipeline cache
			1, // number of pipeline createe info
			&graphicsPipelineCreateInfo,  
			nullptr, // allocation callbacks 
			&graphicsPipeline);
	handleVulkanError(graphicsPipelineCreationResult, "vkCreateGraphicsPipelines", true);
	vkDestroyShaderModule(device, shaderModule, nullptr);
}
