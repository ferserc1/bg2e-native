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
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/vulkan/rt/RayTracingVertexInfo.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

template <typename MeshT>
class BG2E_API RayTracingMeshGeneric {
public:
    RayTracingMeshGeneric(Engine* engine, geo::MeshGeneric<MeshT>* mesh, uint32_t firstIndex, uint32_t indexCount);
    ~RayTracingMeshGeneric();

    void build();

    bool isBuilt() const;

    uint32_t firstIndex() const;
    uint32_t indexCount() const;
    uint32_t triangleCount() const;

    VkDeviceAddress vertexDeviceAddress() const;
    VkDeviceAddress indexDeviceAddress() const;

    VkFormat positionFormat() const;
    uint32_t vertexStride() const;
    VkDeviceSize positionOffset() const;

    VkAccelerationStructureKHR handle() const;
    VkDeviceAddress deviceAddress() const;

private:
    void cleanup(); // Called from destructor

private:
    Engine* _engine = nullptr;
    geo::MeshGeneric<MeshT>* _mesh = nullptr;
    uint32_t _firstIndex = 0;
    uint32_t _indexCount = 0;

    VkAccelerationStructureKHR _blas = VK_NULL_HANDLE;
    std::unique_ptr<Buffer> _blasBuffer;
    VkDeviceAddress _blasDeviceAddress = 0;
};

typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshP> RayTracingMeshP;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPN> RayTracingMeshPN;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPC> RayTracingMeshPC;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPU> RayTracingMeshPU;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNU> RayTracingMeshPNU;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNC> RayTracingMeshPNC;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNUC> RayTracingMeshPNUC;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNUT> RayTracingMeshPNUT;
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNUUT> RayTracingMeshPNUUT;

// Default mesh type
typedef BG2E_API RayTracingMeshGeneric<bg2e::geo::Mesh> RayTracingMesh;

}
}
}
}
