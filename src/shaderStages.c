#include <stdio.h>
#include <stdlib.h>
#include "readFile.h"
#include <vulkan/vulkan_core.h>
#include "rendererErrors.h"
#include "utils/functionQueue.h"

#define SHADER_DIR "shaders/"

extern VkDevice device;
extern VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;


static VkShaderModule shaderModule;


extern struct functionStack cleanupFunctions;

static RendererResult createShaderModule(const uint32_t *shaderCode, const size_t shaderCodeSize, VkShaderModule *shaderModule) {
	// shader code needs to be 32 bit aligned
	if ((uint64_t) shaderCode % 4 != 0) {
		RR_SET_ERROR(RENDERER_ERR_SHADERS, 0, "spir-v shader code is not 4 byte aligned");
		return RENDERER_ERR_SHADERS;
	}

	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.codeSize = shaderCodeSize;
	createInfo.pCode = shaderCode;
	VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, shaderModule));

	return RENDERER_SUCCESS;
}


static void destroyShaders() {
	vkDestroyShaderModule(device, shaderModule, nullptr);
}

RendererResult initShaders() {
	RendererResult result = RENDERER_SUCCESS;

	char *filepath = SHADER_DIR "triangle.spv";
	uint32_t *shaderCode = nullptr;
	size_t shaderCodeSize = 0;
	RF_CHECK(readFile(filepath, nullptr, &shaderCodeSize));
	shaderCode = malloc(sizeof(char)*shaderCodeSize);
	MALLOC_CHECK(shaderCode);
	RF_CHECK(readFile(filepath, (char *) shaderCode, &shaderCodeSize), result, cleanup);

	result = createShaderModule(shaderCode, shaderCodeSize, &shaderModule);
	if (result) goto cleanup;

	functionStack_insert(&cleanupFunctions, destroyShaders);


	constexpr int shaderStagesCount = 2;

	static VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
	static VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};

	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";


	static VkPipelineShaderStageCreateInfo shaderStages[shaderStagesCount];
	shaderStages[0] = vertShaderStageInfo;
	shaderStages[1] = fragShaderStageInfo;

	graphicsPipelineCreateInfo.stageCount = shaderStagesCount;
	graphicsPipelineCreateInfo.pStages = shaderStages;

cleanup:
	if(shaderCode) free(shaderCode);

	return result;
}
