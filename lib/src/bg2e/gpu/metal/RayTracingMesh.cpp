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

#include <bg2e/gpu/metal/RayTracingMesh.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/Buffer.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

RayTracingMesh::RayTracingMesh(metal::Device* device, const RayTracingMeshDescription& description)
    : gpu::RayTracingMesh(device)
    , _device(device)
{
    _description = description;

    if (!dynamic_cast<metal::Buffer*>(description.vertexBuffer) ||
        !dynamic_cast<metal::Buffer*>(description.indexBuffer))
    {
        throw std::runtime_error("metal::RayTracingMesh: vertex/index buffers must be metal::Buffer objects");
    }
    if (description.indexCount == 0 || (description.indexCount % 3) != 0)
    {
        throw std::runtime_error("metal::RayTracingMesh: indexCount must be a non-zero multiple of 3");
    }
}

RayTracingMesh::~RayTracingMesh()
{
    cleanup();
}

bool RayTracingMesh::isValid() const
{
    return _accelerationStructure != nullptr;
}

MTL::PrimitiveAccelerationStructureDescriptor* RayTracingMesh::buildDescriptor() const
{
    auto* vertexBuffer = dynamic_cast<metal::Buffer*>(_description.vertexBuffer)->handle();
    auto* indexBuffer  = dynamic_cast<metal::Buffer*>(_description.indexBuffer)->handle();

    auto* triangles = MTL::AccelerationStructureTriangleGeometryDescriptor::alloc()->init();
    triangles->setVertexBuffer(vertexBuffer);
    triangles->setVertexBufferOffset(_description.positionOffset);
    triangles->setVertexStride(_description.vertexStride);
    triangles->setIndexBuffer(indexBuffer);
    triangles->setIndexBufferOffset(_description.firstIndex * sizeof(uint32_t));
    triangles->setIndexType(MTL::IndexTypeUInt32);
    triangles->setTriangleCount(_description.indexCount / 3);
    triangles->setOpaque(true);

    const NS::Object* geometries[] = { triangles };
    NS::Array* geometryArray = NS::Array::array(geometries, 1);

    auto* descriptor = MTL::PrimitiveAccelerationStructureDescriptor::alloc()->init();
    descriptor->setGeometryDescriptors(geometryArray);

    triangles->release();
    return descriptor;
}

void RayTracingMesh::build(MTL::CommandBuffer* cmd)
{
    auto* descriptor = buildDescriptor();

    MTL::AccelerationStructureSizes sizes = _device->handle()->accelerationStructureSizes(descriptor);

    if (!_accelerationStructure)
    {
        _accelerationStructure = _device->handle()->newAccelerationStructure(sizes.accelerationStructureSize);
        if (!_accelerationStructure)
        {
            descriptor->release();
            throw std::runtime_error("metal::RayTracingMesh: failed to create acceleration structure");
        }
    }

    if (!_scratchBuffer || _scratchSize < sizes.buildScratchBufferSize)
    {
        if (_scratchBuffer) { _scratchBuffer->release(); }
        _scratchBuffer = _device->handle()->newBuffer(sizes.buildScratchBufferSize,
                                                      MTL::ResourceStorageModePrivate);
        _scratchSize = sizes.buildScratchBufferSize;
    }

    MTL::AccelerationStructureCommandEncoder* encoder = cmd->accelerationStructureCommandEncoder();
    encoder->buildAccelerationStructure(_accelerationStructure, descriptor, _scratchBuffer, 0);
    encoder->endEncoding();

    descriptor->release();
    _built = true;
}

void RayTracingMesh::cleanup()
{
    if (_scratchBuffer)
    {
        _scratchBuffer->release();
        _scratchBuffer = nullptr;
    }
    if (_accelerationStructure)
    {
        _accelerationStructure->release();
        _accelerationStructure = nullptr;
    }
    _built = false;
}

#else

RayTracingMesh::RayTracingMesh(metal::Device* device, const RayTracingMeshDescription& description)
    : gpu::RayTracingMesh(device)
    , _device(device)
{
    _description = description;
    throw std::runtime_error("Metal backend is not available on this platform");
}

RayTracingMesh::~RayTracingMesh() {}
void RayTracingMesh::cleanup() {}
bool RayTracingMesh::isValid() const { return false; }

#endif

}
}
}
