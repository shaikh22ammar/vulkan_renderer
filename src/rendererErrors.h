#ifndef RENDERER_ERROR_H
#define RENDERER_ERROR_H

typedef enum {
	RENDERER_SUCCESS,
	RENDERER_ERROR_GLFW,
	RENDERER_ERROR_VULKAN_INIT,
	RENDERER_ERROR_GRAPHICS_PIPELINE
} RendererExitCode;

char *rendererPrintError(RendererExitCode e);
#endif
