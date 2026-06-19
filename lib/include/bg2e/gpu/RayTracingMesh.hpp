/*
 *    business grade graphic engine (bg2e engine)
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
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>

#include <cstdint>
#include <string>

namespace bg2e {
namespace gpu {

class Buffer;

// Bottom-level ray tracing geometry, built from existing GPU vertex/index
// buffers of one submesh.
//
// RayTracingMesh is created from buffers that already live on the GPU (the same
// vertex/index buffers used for rasterization). It does NOT own these buffers;
// the caller is responsible for keeping them alive while the RayTracingMesh is
// in use.
//
// The recommended way to fill a RayTracingMeshDescription is the helper
// MeshGeneric<MeshT>::rayTracingMeshDescription(submeshIndex), which resolves
// the vertex stride, position offset and submesh index range automatically.
struct RayTracingMeshDescription {
    // Existing GPU buffers shared with the rasterization mesh.
    gpu::Buffer* vertexBuffer = nullptr;   // interleaved vertex buffer
    gpu::Buffer* indexBuffer  = nullptr;   // 32-bit index buffer

    uint32_t vertexCount    = 0;           // number of vertices in the vertex buffer
    uint32_t vertexStride   = 0;           // bytes between consecutive vertices
    uint32_t positionOffset = 0;           // byte offset of the position attribute
    Format   vertexFormat   = Format::R32G32B32_SFLOAT; // position format

    // Submesh index range inside the shared index buffer.
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;

    std::string debugName;
};

class BG2E_API RayTracingMesh : public DeviceResource {
public:
    explicit RayTracingMesh(Device* device) : DeviceResource(device) {}
    ~RayTracingMesh() override = default;

    const RayTracingMeshDescription& description() const { return _description; }

    // True once the acceleration structure has been built on the GPU.
    virtual bool isBuilt() const = 0;

protected:
    RayTracingMeshDescription _description;
};

}
}
