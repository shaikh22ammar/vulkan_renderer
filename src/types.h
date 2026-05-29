#ifndef TYPES_H
#define TYPES_H

#include <cglm/cglm.h>

struct pushConstants {
	mat4 mvp;
};

struct Vertex {
	vec3 pos;
	vec2 uv;
};


#endif

