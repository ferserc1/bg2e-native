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

#include <bg2e/render/vulkan/rt/RayTracingMesh.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>

namespace bg2e::render::vulkan::rt {

template <typename MeshT>
RayTracingMesh<MeshT>::RayTracingMesh(geo::MeshGeneric<MeshT>* mesh, uint32_t firstIndex, uint32_t indexCount)
    : _mesh{ mesh }, _firstIndex{ firstIndex }, _indexCount{ indexCount }
{
}

template <typename MeshT>
VkDeviceAddress RayTracingMesh<MeshT>::vertexDeviceAddress() const
{
    return _mesh->vertexDeviceAddress();
}

template <typename MeshT>
VkDeviceAddress RayTracingMesh<MeshT>::indexDeviceAddress() const
{
    return _mesh->indexDeviceAddress();
}

template <typename MeshT>
VkFormat RayTracingMesh<MeshT>::positionFormat() const
{
    return RayTracingVertexInfo<MeshT>::kPositionFormat;
}

template <typename MeshT>
uint32_t RayTracingMesh<MeshT>::vertexStride() const
{
    return static_cast<uint32_t>(RayTracingVertexInfo<MeshT>::kStride);
}

template <typename MeshT>
VkDeviceSize RayTracingMesh<MeshT>::positionOffset() const
{
    return RayTracingVertexInfo<MeshT>::kPositionOffset;
}

template <typename MeshT>
uint32_t RayTracingMesh<MeshT>::firstIndex() const
{
    return _firstIndex;
}

template <typename MeshT>
uint32_t RayTracingMesh<MeshT>::indexCount() const
{
    return _indexCount;
}

template <typename MeshT>
uint32_t RayTracingMesh<MeshT>::triangleCount() const
{
    return _indexCount / 3;
}

template class RayTracingMesh<bg2e::geo::MeshP>;
template class RayTracingMesh<bg2e::geo::MeshPN>;
template class RayTracingMesh<bg2e::geo::MeshPC>;
template class RayTracingMesh<bg2e::geo::MeshPU>;
template class RayTracingMesh<bg2e::geo::MeshPNU>;
template class RayTracingMesh<bg2e::geo::MeshPNC>;
template class RayTracingMesh<bg2e::geo::MeshPNUC>;
template class RayTracingMesh<bg2e::geo::MeshPNUT>;
template class RayTracingMesh<bg2e::geo::MeshPNUUT>;

}
