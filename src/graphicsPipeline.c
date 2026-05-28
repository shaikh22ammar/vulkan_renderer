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
#include "rendererErrors.h"
#include <vulkan/vulkan_core.h>
#include "utils/functionQueue.h"

extern VkDevice device;

extern VkExtent2D swapChainExtent;
extern VkSurfaceFormatKHR swapChainSurfaceFormat;


extern VkPipeline graphicsPipeline;
extern VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
extern VkPipelineLayout pipelineLayout;
extern VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;

extern VkPushConstantsInfo pushConstantsInfo;



extern struct functionStack cleanupFunctions;




static void defaultPipelineLayoutCreateInfo() {
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
}

static void defaultGraphicsPipelineCreateInfo() {
	constexpr int dynamicStatesCount = 2;
	static VkDynamicState dynamicStates[dynamicStatesCount] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	static VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.dynamicStateCount = dynamicStatesCount,
		.pDynamicStates = dynamicStates
	};

	static VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.viewportCount = 1,
		.scissorCount = 1
	};

// input assembly
	static VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.primitiveRestartEnable = VK_FALSE,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	};
 
// rasterization
	static VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,//VK_CULL_MODE_BACK_BIT;
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

// multisample
	static VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};
// color blending
// After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. 
	static VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
				| VK_COLOR_COMPONENT_G_BIT
				| VK_COLOR_COMPONENT_B_BIT
				| VK_COLOR_COMPONENT_A_BIT,
	};

	static VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState
	};

	static VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &swapChainSurfaceFormat.format,
		.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT
	};

	static VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					// This is needed since render pass is not used
	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
	graphicsPipelineCreateInfo.stageCount = 0;
	graphicsPipelineCreateInfo.pStages = nullptr;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	graphicsPipelineCreateInfo.pVertexInputState = nullptr;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStatesCreateInfo;
	graphicsPipelineCreateInfo.layout = nullptr;
	graphicsPipelineCreateInfo.renderPass = nullptr;
}

RendererResult initGraphicsPipelineCreateInfo() {
	defaultGraphicsPipelineCreateInfo();
	defaultPipelineLayoutCreateInfo();
	return RENDERER_SUCCESS;
}

void destroyPipelines() {
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
}

RendererResult createGraphicsPipeline() {
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline));

	functionStack_insert(&cleanupFunctions, destroyPipelines);
	return RENDERER_SUCCESS;
}
