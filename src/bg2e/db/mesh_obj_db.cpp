

#include <bg2e/db/mesh_obj.hpp>

#include <bg2e/base/Log.hpp>

#include <glm/glm.hpp>

#include <bg2e/geo/modifiers.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#include <fstream>
#include <functional>

namespace bg2e::db {

template <class MeshT>
MeshT* loadMeshObj(std::istream& inputStream) {
	throw std::runtime_error("Unsupported mesh type");
}

void readObjMesh(
	std::istream& inputStream,
	std::function<void(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)> vertexCallback, 
	std::function<void(uint32_t index)> indexCallback,
	std::function<void(uint32_t offset, uint32_t count)> submeshCallback
) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream);
	if (!warn.empty()) {
		bg2e_log_warning << "Warning loading OBJ file: " << warn << bg2e_log_end;
	}

	if (!err.empty()) {
		throw std::runtime_error(std::string("Error loading OBJ file: ") + err);
	}
 
    uint32_t currentIndex = 0;
    uint32_t submeshOffset = 0;
    for (const auto& shape : shapes)
    {
        uint32_t submeshCount = 0;
        for (const auto& index : shape.mesh.indices)
        {
            // Curretly, the loader only supports triangles
            
            glm::vec3 position {
                attrib.vertices[3 * index.vertex_index],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            glm::vec3 normal {
                attrib.normals[3 * index.normal_index],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };
            glm::vec2 uv {
                attrib.texcoords[2 * index.texcoord_index],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            
            vertexCallback(position, normal, uv);
            indexCallback(currentIndex++);
            ++submeshCount;
        }
        submeshCallback(submeshOffset, submeshCount);
        submeshOffset = currentIndex;
    }
}

template <>
bg2e::geo::MeshP* loadMeshObj<bg2e::geo::MeshP>(std::istream& inputStream) {
	return nullptr;
}

template <>
bg2e::geo::MeshPN* loadMeshObj<bg2e::geo::MeshPN>(std::istream& inputStream) {
	auto mesh = new bg2e::geo::MeshPN();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPN{ position, normal });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);
	return mesh;
}

template <>
bg2e::geo::MeshPC* loadMeshObj<bg2e::geo::MeshPC>(std::istream& inputStream) {
	auto mesh = new bg2e::geo::MeshPC();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPC{ position, { 1.0f, 1.0f, 1.0f, 1.0f } });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);
	return mesh;
}

template <>
bg2e::geo::MeshPU* loadMeshObj<bg2e::geo::MeshPU>(std::istream& inputStream) {
	auto mesh = new bg2e::geo::MeshPU();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPU{ position, uv });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);
	return mesh;
}

template <>
bg2e::geo::MeshPNU* loadMeshObj<bg2e::geo::MeshPNU>(std::istream& inputStream) {
	auto mesh = new bg2e::geo::MeshPNU();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPNU{ position, normal, uv });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);
	return mesh;
}

template <>
bg2e::geo::MeshPNC* loadMeshObj<bg2e::geo::MeshPNC>(std::istream& inputStream) {
	auto mesh = new bg2e::geo::MeshPNC();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPNC{ position, normal, { 1.0f, 1.0f, 1.0f, 1.0f } });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);
	return mesh;
}

template <>
bg2e::geo::MeshPNUC* loadMeshObj<bg2e::geo::MeshPNUC>(std::istream& inputStream) {
	auto mesh = new bg2e::geo::MeshPNUC();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPNUC{ position, normal, uv, { 1.0f, 1.0f, 1.0f, 1.0f } });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);
	return mesh;
}

template <>
bg2e::geo::MeshPNUT* loadMeshObj<bg2e::geo::MeshPNUT>(std::istream& inputStream)
{
	auto mesh = new bg2e::geo::MeshPNUT();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPNUT{ position, normal, uv, { 0.0f, 0.0f, 0.0f } });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);

    geo::GenTangentsModifier<geo::MeshPNUT> tangentGenerator(mesh);
    tangentGenerator.apply();

	return mesh;
}

template <>
bg2e::geo::MeshPNUUT* loadMeshObj<bg2e::geo::MeshPNUUT>(std::istream& inputStream)
{
	auto mesh = new bg2e::geo::MeshPNUUT();

	readObjMesh(
		inputStream,
		[&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
			mesh->vertices.push_back(bg2e::geo::VertexPNUUT{ position, normal, uv, uv, { 0.0f, 0.0f, 0.0f } });
		},
		[&](uint32_t index) {
			mesh->indices.push_back(index);
		},
		[&](uint32_t offset, uint32_t count) {
			mesh->submeshes.push_back({ offset, count });
		}
	);

    geo::GenTangentsModifier<geo::MeshPNUUT> tangentGenerator(mesh);
    tangentGenerator.apply();

	return mesh;
}


}
