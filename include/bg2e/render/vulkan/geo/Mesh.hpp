#pragma once

#include <bg2e/common.hpp>
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>

#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {
namespace geo {

template <typename MeshT>
class BG2E_API MeshGeneric {
public:
	MeshGeneric(Vulkan* vulkan) :_vulkan{ vulkan } {}

    inline MeshT & meshData() { return _meshData; }
    inline const MeshT & meshData() const { return _meshData; }
    inline void setMeshData(const MeshT & m) { _meshData = m; }
    inline void setMeshData(const MeshT * m) { _meshData = *m; }
	inline uint32_t submeshCount() const { return uint32_t(_meshData.submeshes.size()); }

	inline const Buffer* vertexBuffer() const { return _vertexBuffer.get(); }
	inline const Buffer* indexBuffer() const { return _indexBuffer.get(); }

	void build();
	void draw(VkCommandBuffer cmd);
	void drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
	void cleanup();

protected:
	Vulkan* _vulkan;

    MeshT _meshData;

	std::unique_ptr<Buffer> _vertexBuffer;
	std::unique_ptr<Buffer> _indexBuffer;
};

typedef BG2E_API MeshGeneric<bg2e::geo::MeshP> MeshP;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPN> MeshPN;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPC> MeshPC;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPU> MeshPU;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPNU> MeshPNU;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPNC> MeshPNC;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPNUC> MeshPNUC;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPNUT> MeshPNUT;
typedef BG2E_API MeshGeneric<bg2e::geo::MeshPNUUT> MeshPNUUT;

// Default mesh type
typedef BG2E_API MeshGeneric<bg2e::geo::Mesh> Mesh;

}
}
}
}
