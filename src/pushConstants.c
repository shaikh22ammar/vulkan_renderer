#include "rendererErrors.h"
#include "vulkan/vulkan_core.h"
#include <cglm/cglm.h>
#include "types.h"

extern VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
extern VkPipelineLayout pipelineLayout;

extern struct pushConstants pushConstants;

void updatePushConstants() {
	//TODO
	glm_mat4_identity(pushConstants.mvp);
}

RendererResult initPushConstants() {
	glm_mat4_identity(pushConstants.mvp);

	constexpr size_t sizeOfPushConstants = sizeof(struct pushConstants);
	static VkPushConstantRange pcRange = {
		.size = sizeOfPushConstants,
		.offset = 0,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	};
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

	return RENDERER_SUCCESS;
}
