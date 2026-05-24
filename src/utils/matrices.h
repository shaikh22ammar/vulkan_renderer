#ifndef UTILS_H
#define UTILS_H

#include <cglm/clipspace/persp_lh_zo.h>

static inline void glm_perspective_vk(float fovy, float aspect, float nearVal, float farVal, mat4 dest) {
	/* A perspective matrix based on Vulkan's coordinate system:
	 * y down, z forw, x right. It simply is a wrapper for cglm's perspective
	 * matrix when left handed coordinate system and depth from 0 to 1 
	 * option is enabled. This works even if Vulkan's coordinate system is right-handed,
	 * since in cglm, z looking forward is equivalent to "left-handed coordinate system" 
	 * as far as the perspective matrix is concerned */
	glm_perspective_lh_zo(fovy, aspect, nearVal, farVal, dest);
}

#endif
