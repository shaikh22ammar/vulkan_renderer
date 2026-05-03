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
	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.codeSize = shaderCodeSize;
	createInfo.pCode = shaderCode;
	return vkCreateShaderModule(device, &createInfo, nullptr, shaderModule);
}

void createGraphicsPipeline() {
	char *filepath = SHADER_DIR "triangle.spv";
	uint32_t *shaderCode = nullptr;
	size_t shaderCodeSize = 0;
	ReadFileResult shaderCodeReadingResult;
	if ((shaderCodeReadingResult = readFile(filepath, nullptr, &shaderCodeSize)) < READ_FILE_SUCCESS) {
		fprintf(stderr, "ERROR: readFile() exited with error code: %d\n", shaderCodeReadingResult);
		exit(RENDERER_ERROR_GRAPHICS_PIPELINE);
	} else if (shaderCodeReadingResult > READ_FILE_SUCCESS) {
		fprintf(stderr, "WARNING: readFile() returned %d\n", shaderCodeReadingResult);
	}
	shaderCode = malloc(sizeof(char)*shaderCodeSize);
	if (!shaderCode) {
		fprintf(stderr, "ERROR: Failed to allocated memory for shader code\n");
		exit(RENDERER_ERROR_GRAPHICS_PIPELINE);
	}
	if ((shaderCodeReadingResult = readFile(filepath, (char *) shaderCode, &shaderCodeSize)) < READ_FILE_SUCCESS) {
		fprintf(stderr, "ERROR: readFile() exited with error code: %d\n", shaderCodeReadingResult);
		exit(RENDERER_ERROR_GRAPHICS_PIPELINE);
	} else if (shaderCodeReadingResult > READ_FILE_SUCCESS) {
		fprintf(stderr, "WARNING: readFile() returned %d\n", shaderCodeReadingResult);
	}

	VkShaderModule shaderModule;
	VkResult shaderModuleCreationResult;
	if ((shaderModuleCreationResult = createShaderModule(shaderCode, shaderCodeSize, &shaderModule)) < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreateShaderModule() returned error code %d\n", shaderCodeReadingResult);
		exit(RENDERER_ERROR_GRAPHICS_PIPELINE);
	} else if (shaderModuleCreationResult > VK_SUCCESS) {
		fprintf(stderr, "WARNING: vkCreateShaderModule() returned %d\n", shaderModuleCreationResult);
	}
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

	// Dynamic states and viewports
	constexpr int dynamicStatesCount = 2;
	VkDynamicState dynamicStates[dynamicStatesCount] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo = {0};
	dynamicStatesCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStatesCreateInfo.pNext = nullptr;
	dynamicStatesCreateInfo.dynamicStateCount = dynamicStatesCount;
	dynamicStatesCreateInfo.pDynamicStates = dynamicStates;

	/*
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.height = (float) swapChainExtent.height;
	viewport.width = (float) swapChainExtent.width;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D) {.x = 0, .y = 0};
	scissor.extent = swapChainExtent;
	*/
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
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.lineWidth = 1.0f;

	// multisample
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {0};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext = nullptr;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {0};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
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
	if ((pipelineLayoutCreationResult = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)) 
			< VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreatePipelineLayout() returned %d\n", pipelineLayoutCreationResult);
		exit(RENDERER_ERROR_GRAPHICS_PIPELINE);
	} else if (pipelineLayoutCreationResult > VK_SUCCESS) {
		fprintf(stderr, "WARNING: vkCreatePipelineLayout() returned %d\n", pipelineLayoutCreationResult);
	}

	// create graphics pipeline
	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {0};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipelineRenderingCreateInfo.pNext = nullptr;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainSurfaceFormat.format;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {0};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
	graphicsPipelineCreationResult = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);
	
	if (graphicsPipelineCreationResult < VK_SUCCESS) {
		fprintf(stderr, "ERROR: vkCreateGraphicssPipelines() returned %d\n", graphicsPipelineCreationResult);
		exit(RENDERER_ERROR_GRAPHICS_PIPELINE);
	} else if (graphicsPipelineCreationResult > VK_SUCCESS) {
		fprintf(stderr, "WARNING: vkCreateGraphicsPipeline() returned %d\n", graphicsPipelineCreationResult);
	}

	vkDestroyShaderModule(device, shaderModule, nullptr);
}
