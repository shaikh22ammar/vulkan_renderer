#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include "types.h"

struct VertexPP {
	glm::vec3 pos;
	glm::vec2 uv;
	bool operator==(const struct VertexPP& other) const {
	    return pos == other.pos && uv == other.uv;
	}
};


namespace std {
    template<> struct hash<struct VertexPP> {
        size_t operator()(struct VertexPP const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}
std::unordered_map<struct VertexPP, uint32_t> uniqueVertices{};

static std::vector<struct VertexPP> verticesPP;
static std::vector<uint32_t> indicesPP;

extern "C" void loadModel(struct Vertex **vertices, uint32_t **indices, int *numVertices, int *numIndices) {
	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;
	std::string                      warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, ASSET_DIR "/models/viking_room.obj")) {
		throw std::runtime_error(warn + err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			struct VertexPP vertex{};
			/* The model is using (k, i, j) basis,
			 * but Vulkan uses (i, -j, -k) basis */
			vertex.pos[0] =  1.0f * attrib.vertices[3 * index.vertex_index + 1];
			vertex.pos[1] = -1.0f * attrib.vertices[3 * index.vertex_index + 2];
			vertex.pos[2] = -1.0f * attrib.vertices[3 * index.vertex_index + 0];

			vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
			vertex.uv[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

			auto [it, inserted] = uniqueVertices.insert({vertex, static_cast<uint32_t>(verticesPP.size())});
			if (inserted) {
				verticesPP.push_back(vertex);
			}
			indicesPP.push_back(it->second);
		}
	}

	*vertices = (struct Vertex *) verticesPP.data();
	*indices = indicesPP.data();
	*numVertices = verticesPP.size();
	*numIndices = indicesPP.size();
}
