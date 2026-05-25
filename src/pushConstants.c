#include "rendererErrors.h"
#include "vulkan/vulkan_core.h"
#include <cglm/cglm.h>
#include "types.h"
#include <time.h>

extern VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
extern VkPipelineLayout pipelineLayout;


RendererResult initPushConstants() {
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
