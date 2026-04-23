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

#include <stdexcept>

#include <bg2e/render/vulkan/rt/RayTracingMesh.hpp>
#include <bg2e/render/vulkan/rt/RayTracingVertexInfo.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Device.hpp>
#include <bg2e/render/vulkan/rt/RTAccelerationStructureSize.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

template <typename MeshT>
RayTracingMeshGeneric<MeshT>::RayTracingMeshGeneric(Engine* engine,
												   geo::MeshGeneric<MeshT>* mesh,
												   uint32_t firstIndex,
												   uint32_t indexCount)
	: _engine{ engine }
	, _mesh{ mesh }
	, _firstIndex{ firstIndex }
	, _indexCount{ indexCount }
{
}

template <typename MeshT>
RayTracingMeshGeneric<MeshT>::~RayTracingMeshGeneric()
{
	cleanup();
}

template <typename MeshT>
void RayTracingMeshGeneric<MeshT>::build()
{
	if (isBuilt()) {
		return;
	}

	if (!_mesh) {
		throw std::runtime_error("RayTracingMeshGeneric::build(): mesh is null");
	}

	if (_mesh->meshData().vertices.empty() || _mesh->meshData().indices.empty()) {
		throw std::runtime_error("RayTracingMeshGeneric::build(): mesh data is empty");
	}

	if (_indexCount == 0) {
		throw std::runtime_error("RayTracingMeshGeneric::build(): indexCount is zero");
	}

	if ((_indexCount % 3) != 0) {
		throw std::runtime_error("RayTracingMeshGeneric::build(): indexCount is not a multiple of 3");
	}

    if (!_engine->rayTracingSupported())
    {
        throw std::runtime_error("RayTracingMeshGeneric::build(): Ray tracing is not supported in this system");
    }

    RTAccelerationStructureSize size(_engine);
    auto buildSizesInfo = size.getBLASSizes(
        vertexDeviceAddress(),
        indexDeviceAddress(),
        positionFormat(),
        vertexStride(),
        static_cast<uint32_t>(_mesh->meshData().vertices.size() - 1),
        triangleCount(),
        VK_INDEX_TYPE_UINT32
    );

    _blasBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
        _engine,
        buildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "BLAS buffer"
    ));

    std::unique_ptr<Buffer> scratchBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
        _engine,
        buildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "BLAS scratch buffer"
    ));

    VkAccelerationStructureCreateInfoKHR blasInfo{};
    blasInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    blasInfo.buffer = _blasBuffer->handle();
    blasInfo.offset = 0;
    blasInfo.size = buildSizesInfo.accelerationStructureSize;
    blasInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (createAccelerationStructure(
        _engine->device().handle(),
        &blasInfo,
        nullptr,
        &_blas
    ) != VK_SUCCESS)
    {
        cleanup();
        throw std::runtime_error("failed to create acceleration structure!");
    }

    // Complete the buildInfo to build the BLAS
    auto buildInfo = size.buildInfo();
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = _blas;
    buildInfo.scratchData.deviceAddress = scratchBuffer->deviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount = triangleCount();
    buildRangeInfo.primitiveOffset = firstIndex() * sizeof(uint32_t);
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &buildRangeInfo };
    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        cmdBuildAccelerationStructures(
            cmd, 1, &buildInfo, buildRangeInfos
        );
    });

    // Get the BLAS device address
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = _blas;
    _blasDeviceAddress = getAccelerationStructureDeviceAddress(
        _engine->device().handle(),
        &addressInfo
    );
}

template <typename MeshT>
void RayTracingMeshGeneric<MeshT>::cleanup()
{
    if (_blas != VK_NULL_HANDLE)
    {
        destroyAccelerationStructure(
            _engine->device().handle(),
            _blas,
            nullptr
        );
    }
	if (_blasBuffer) {
		_blasBuffer->cleanup();
		_blasBuffer.reset();
	}

	_blas = VK_NULL_HANDLE;
	_blasDeviceAddress = 0;
}

template <typename MeshT>
bool RayTracingMeshGeneric<MeshT>::isBuilt() const
{
	return _blas != VK_NULL_HANDLE;
}

template <typename MeshT>
uint32_t RayTracingMeshGeneric<MeshT>::firstIndex() const
{
	return _firstIndex;
}

template <typename MeshT>
uint32_t RayTracingMeshGeneric<MeshT>::indexCount() const
{
	return _indexCount;
}

template <typename MeshT>
uint32_t RayTracingMeshGeneric<MeshT>::triangleCount() const
{
	return _indexCount / 3;
}

template <typename MeshT>
VkDeviceAddress RayTracingMeshGeneric<MeshT>::vertexDeviceAddress() const
{
	return _mesh ? _mesh->vertexDeviceAddress() : 0;
}

template <typename MeshT>
VkDeviceAddress RayTracingMeshGeneric<MeshT>::indexDeviceAddress() const
{
	return _mesh ? _mesh->indexDeviceAddress() : 0;
}

template <typename MeshT>
VkFormat RayTracingMeshGeneric<MeshT>::positionFormat() const
{
	return RayTracingVertexInfo<MeshT>::kPositionFormat;
}

template <typename MeshT>
uint32_t RayTracingMeshGeneric<MeshT>::vertexStride() const
{
	return static_cast<uint32_t>(RayTracingVertexInfo<MeshT>::kStride);
}

template <typename MeshT>
VkDeviceSize RayTracingMeshGeneric<MeshT>::positionOffset() const
{
	return RayTracingVertexInfo<MeshT>::kPositionOffset;
}

template <typename MeshT>
VkAccelerationStructureKHR RayTracingMeshGeneric<MeshT>::handle() const
{
	return _blas;
}

template <typename MeshT>
VkDeviceAddress RayTracingMeshGeneric<MeshT>::deviceAddress() const
{
	return _blasDeviceAddress;
}

// Explicit template instantiations

template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshP>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPN>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPC>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPU>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNU>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNC>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNUC>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNUT>;
template class BG2E_API RayTracingMeshGeneric<bg2e::geo::MeshPNUUT>;

}
}
}
}