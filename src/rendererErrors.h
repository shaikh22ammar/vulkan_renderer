#ifndef RENDERER_ERROR_H
#define RENDERER_ERROR_H

#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <stdio.h>

typedef enum {
	RENDERER_SUCCESS,
	RENDERER_ERROR_OUT_OF_MEMORY,
	RENDERER_ERROR_VULKAN,
	RENDERER_ERROR_GFLW,
	RENDERER_ERROR_IO,
	RENDERER_ERROR_UNKNOWN
} RendererExitCode;

static inline void handleVulkanError(const VkResult result, const char *functionName, const bool exitIfError) {
	if (result < VK_SUCCESS) {
		fprintf(stderr, "VULKAN ERROR: %s returned %d\n", functionName, result);
		if (exitIfError) exit(RENDERER_ERROR_VULKAN);
	} else if (result > VK_SUCCESS) {
		fprintf(stderr, "VULKAN WARNING: %s returned %d\n", functionName, result);
	}
}

static inline void handleRendererError(const RendererExitCode result, const char *functionName, const bool exitIfError) {
	if (result != RENDERER_SUCCESS) {
		fprintf(stderr, "RENDERER ERROR: %s returned %d\n", functionName, result);
		if (exitIfError) exit(RENDERER_ERROR_VULKAN);
	}
}
#endif
