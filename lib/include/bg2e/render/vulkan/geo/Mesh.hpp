/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>

#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {
namespace geo {

template <typename MeshT>
class BG2E_API MeshGeneric {
public:
	MeshGeneric(Engine* engine) :_engine{ engine } {}
    virtual ~MeshGeneric();

    inline MeshT & meshData() { return _meshData; }
    inline const MeshT & meshData() const { return _meshData; }
    inline void setMeshData(const MeshT & m) { _meshData = m; }
    inline void setMeshData(const MeshT * m) { _meshData = *m; }
	inline uint32_t submeshCount() const { return uint32_t(_meshData.submeshes.size()); }

	inline const Buffer* vertexBuffer() const { return _vertexBuffer.get(); }
	inline const Buffer* indexBuffer() const { return _indexBuffer.get(); }

	void build();
	void draw(VkCommandBuffer cmd);
    void draw(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        const std::vector<VkDescriptorSet> &ds,
        VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS
    );
	void drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
    void drawSubmesh(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        const std::vector<VkDescriptorSet> &ds,
        uint32_t submeshIndex,
        VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS
    );   

	static VkVertexInputBindingDescription bindingDescription();
	static std::vector<VkVertexInputAttributeDescription> attributeDescriptions();

    void cleanup();

protected:
	Engine* _engine;

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
