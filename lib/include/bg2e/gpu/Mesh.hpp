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

#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/geo/Mesh.hpp>

#include <memory>
#include <stdexcept>

namespace bg2e {
namespace gpu {

template <typename MeshT>
class BG2E_API MeshGeneric {
public:
    using geo_type = MeshT;

    MeshT&       meshData()       { return _meshData; }
    const MeshT& meshData() const { return _meshData; }

    void setMeshData(const MeshT& data)  { _meshData = data; }
    void setMeshData(const MeshT* data)  { if (data) _meshData = *data; }

    uint32_t submeshCount() const
    {
        return static_cast<uint32_t>(_meshData.submeshes.size());
    }

    void build(gpu::Device* device)
    {
        _vertexBuffer = device->createBuffer("Mesh vertex buffer");
        _vertexBuffer->createVertexBuffer(_meshData.vertices);

        _indexBuffer = device->createBuffer("Mesh index buffer");
        _indexBuffer->createIndexBuffer(_meshData.indices);
    }

    void draw(gpu::CommandBuffer* cmd)
    {
        cmd->bindVertexBuffer(0, _vertexBuffer.get(), 0);
        cmd->bindIndexBuffer(_indexBuffer.get(), 0);

        for (uint32_t i = 0; i < submeshCount(); ++i)
        {
            drawSubmesh(cmd, i);
        }
    }

    void drawSubmesh(gpu::CommandBuffer* cmd, uint32_t submeshIndex)
    {
        if (submeshIndex >= submeshCount())
        {
            throw std::runtime_error("gpu::MeshGeneric::drawSubmesh: submesh index out of range");
        }

        const auto& submesh = _meshData.submeshes[submeshIndex];
        cmd->drawIndexed(submesh.indexCount, 1, submesh.firstIndex, 0, 0);
    }

    static gpu::VertexBufferDescription vertexBufferDescription();

    // Build a RayTracingMeshDescription for one submesh, reusing the vertex and
    // index buffers already created by build(). The vertex stride and position
    // offset are resolved from the vertex layout of this mesh type.
    gpu::RayTracingMeshDescription rayTracingMeshDescription(uint32_t submeshIndex) const
    {
        if (submeshIndex >= submeshCount())
        {
            throw std::runtime_error("gpu::MeshGeneric::rayTracingMeshDescription: submesh index out of range");
        }

        const auto vbDesc = vertexBufferDescription();

        uint32_t positionOffset = 0;
        gpu::Format positionFormat = gpu::Format::R32G32B32_SFLOAT;
        for (const auto& attr : vbDesc.attributes)
        {
            if (attr.semantic == gpu::VertexSemantic::Position)
            {
                positionOffset = attr.offset;
                positionFormat = attr.format;
                break;
            }
        }

        const auto& submesh = _meshData.submeshes[submeshIndex];

        gpu::RayTracingMeshDescription desc;
        desc.vertexBuffer   = _vertexBuffer.get();
        desc.indexBuffer    = _indexBuffer.get();
        desc.vertexCount    = static_cast<uint32_t>(_meshData.vertices.size());
        desc.vertexStride   = vbDesc.stride;
        desc.positionOffset = positionOffset;
        desc.vertexFormat   = positionFormat;
        desc.firstIndex     = submesh.firstIndex;
        desc.indexCount     = submesh.indexCount;
        return desc;
    }

    gpu::Buffer*       vertexBuffer()       { return _vertexBuffer.get(); }
    const gpu::Buffer* vertexBuffer() const { return _vertexBuffer.get(); }

    gpu::Buffer*       indexBuffer()       { return _indexBuffer.get(); }
    const gpu::Buffer* indexBuffer() const { return _indexBuffer.get(); }

    void cleanup()
    {
        if (_vertexBuffer) { _vertexBuffer->cleanup(); _vertexBuffer.reset(); }
        if (_indexBuffer)  { _indexBuffer->cleanup();  _indexBuffer.reset(); }
    }

private:
    MeshT                        _meshData;
    std::shared_ptr<gpu::Buffer> _vertexBuffer;
    std::shared_ptr<gpu::Buffer> _indexBuffer;
};

typedef BG2E_API  MeshGeneric<bg2e::geo::MeshP> MeshP;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPN> MeshPN;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPC> MeshPC;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPU> MeshPU;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPNU> MeshPNU;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPNC> MeshPNC;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPNUC> MeshPNUC;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPNUT> MeshPNUT;
typedef BG2E_API  MeshGeneric<bg2e::geo::MeshPNUUT> MeshPNUUT;

typedef BG2E_API  MeshGeneric<bg2e::geo::Mesh> Mesh;

}
}
