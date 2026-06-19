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

#include <bg2e/gpu/metal/RayTracingScene.hpp>
#include <bg2e/gpu/metal/RayTracingMesh.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>

#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace bg2e {
namespace gpu {
namespace metal {

RayTracingScene::RayTracingScene(metal::Device* device, const std::string& debugName)
    : gpu::RayTracingScene(device)
    , _device(device)
    , _debugName(debugName)
{
}

RayTracingScene::~RayTracingScene()
{
    cleanup();
}

void RayTracingScene::buildOrUpdate(gpu::CommandBuffer* cmd)
{
    // Converge both entry points (cmd->buildRayTracingScene and
    // scene->buildOrUpdate) on the command buffer implementation.
    cmd->buildRayTracingScene(this);
}

#if BG2E_IS_MAC

bool RayTracingScene::isValid() const
{
    return _instanceAS != nullptr;
}

namespace {

MTL::PackedFloat4x3 toPackedFloat4x3(const glm::mat4& m)
{
    // glm::mat4 is column-major; PackedFloat4x3 is 4 columns of 3 rows.
    MTL::PackedFloat4x3 out;
    for (int c = 0; c < 4; ++c)
    {
        out.columns[c] = MTL::PackedFloat3(m[c][0], m[c][1], m[c][2]);
    }
    return out;
}

}

void RayTracingScene::build(MTL::CommandBuffer* cmd)
{
    const NS::UInteger instanceCount = static_cast<NS::UInteger>(_instances.size());

    // --- Collect the unique referenced primitive acceleration structures ---
    _referencedPrimitives.clear();
    std::unordered_map<MTL::AccelerationStructure*, uint32_t> primitiveIndex;
    for (const auto& inst : _instances)
    {
        auto* metalMesh = dynamic_cast<metal::RayTracingMesh*>(inst.mesh);
        if (!metalMesh || !metalMesh->isBuilt())
        {
            throw std::runtime_error("metal::RayTracingScene::build: instance references an unbuilt RayTracingMesh");
        }
        MTL::AccelerationStructure* prim = metalMesh->handle();
        if (primitiveIndex.find(prim) == primitiveIndex.end())
        {
            primitiveIndex[prim] = static_cast<uint32_t>(_referencedPrimitives.size());
            _referencedPrimitives.push_back(prim);
        }
    }

    // --- Ensure instance descriptor buffer capacity (shared, CPU writable) ---
    const NS::UInteger descSize = sizeof(MTL::AccelerationStructureInstanceDescriptor);
    if (!_instanceBuffer || instanceCount > _instanceCapacity)
    {
        if (_instanceBuffer) { _instanceBuffer->release(); }
        const NS::UInteger capacity = instanceCount > 0 ? instanceCount : 1;
        _instanceBuffer = _device->handle()->newBuffer(capacity * descSize,
                                                       MTL::ResourceStorageModeShared);
        _instanceCapacity = capacity;
    }

    // --- Fill the instance descriptors ---
    if (instanceCount > 0)
    {
        auto* descriptors = static_cast<MTL::AccelerationStructureInstanceDescriptor*>(_instanceBuffer->contents());
        for (NS::UInteger i = 0; i < instanceCount; ++i)
        {
            const auto& src = _instances[i];
            auto* metalMesh = dynamic_cast<metal::RayTracingMesh*>(src.mesh);

            MTL::AccelerationStructureInstanceDescriptor& dst = descriptors[i];
            dst.transformationMatrix = toPackedFloat4x3(src.transform);
            dst.options = MTL::AccelerationStructureInstanceOptionOpaque |
                          MTL::AccelerationStructureInstanceOptionDisableTriangleCulling;
            dst.mask = src.mask;
            dst.intersectionFunctionTableOffset = 0;
            dst.accelerationStructureIndex = primitiveIndex[metalMesh->handle()];
        }
    }

    // --- Build the instance acceleration structure descriptor ---
    std::vector<NS::Object*> primitiveObjects(_referencedPrimitives.begin(), _referencedPrimitives.end());
    NS::Array* primitiveArray = NS::Array::array(
        reinterpret_cast<const NS::Object* const*>(primitiveObjects.data()),
        static_cast<NS::UInteger>(primitiveObjects.size()));

    auto* descriptor = MTL::InstanceAccelerationStructureDescriptor::alloc()->init();
    descriptor->setInstanceCount(instanceCount);
    descriptor->setInstanceDescriptorBuffer(_instanceBuffer);
    descriptor->setInstanceDescriptorBufferOffset(0);
    descriptor->setInstanceDescriptorStride(descSize);
    descriptor->setInstancedAccelerationStructures(primitiveArray);

    MTL::AccelerationStructureSizes sizes = _device->handle()->accelerationStructureSizes(descriptor);

    if (!_instanceAS || _asSize < sizes.accelerationStructureSize)
    {
        if (_instanceAS) { _instanceAS->release(); }
        _instanceAS = _device->handle()->newAccelerationStructure(sizes.accelerationStructureSize);
        _asSize = sizes.accelerationStructureSize;
    }

    if (!_scratchBuffer || _scratchSize < sizes.buildScratchBufferSize)
    {
        if (_scratchBuffer) { _scratchBuffer->release(); }
        _scratchBuffer = _device->handle()->newBuffer(sizes.buildScratchBufferSize,
                                                      MTL::ResourceStorageModePrivate);
        _scratchSize = sizes.buildScratchBufferSize;
    }

    MTL::AccelerationStructureCommandEncoder* encoder = cmd->accelerationStructureCommandEncoder();
    encoder->buildAccelerationStructure(_instanceAS, descriptor, _scratchBuffer, 0);
    encoder->endEncoding();

    descriptor->release();
}

void RayTracingScene::cleanup()
{
    if (_scratchBuffer)  { _scratchBuffer->release();  _scratchBuffer = nullptr; }
    if (_instanceBuffer) { _instanceBuffer->release(); _instanceBuffer = nullptr; }
    if (_instanceAS)     { _instanceAS->release();     _instanceAS = nullptr; }
    _instanceCapacity = 0;
    _asSize = 0;
    _scratchSize = 0;
    _referencedPrimitives.clear();
    _instances.clear();
}

#else

bool RayTracingScene::isValid() const { return false; }
void RayTracingScene::cleanup() { _instances.clear(); }

#endif

}
}
}
