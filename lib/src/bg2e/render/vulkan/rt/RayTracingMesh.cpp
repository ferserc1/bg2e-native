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
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Device.hpp>

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

	// Vulkan BLAS build will be implemented later.
}

template <typename MeshT>
void RayTracingMeshGeneric<MeshT>::cleanup()
{
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