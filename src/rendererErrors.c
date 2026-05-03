#include "rendererErrors.h"

char *rendererPrintError(RendererExitCode e) {
	switch (e) {
		case RENDERER_SUCCESS:
			return "RENDERER_SUCCESS";
		case RENDERER_ERROR_GLFW:
			return "RENDERER_ERROR_GLFW";
		case RENDERER_ERROR_GRAPHICS_PIPELINE:
			return "RENDERER_ERROR_GRAPHICS_PIPELINE";
		case RENDERER_ERROR_VULKAN_INIT:
			return "RENDERER_ERROR_VULKAN_INIT";
		default:
			return "Unknown error";
	}
}
