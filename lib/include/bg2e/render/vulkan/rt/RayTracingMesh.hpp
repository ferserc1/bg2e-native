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
class BG2E_API RayTracingMesh {
public:
    RayTracingMesh(geo::MeshGeneric<MeshT>* mesh, uint32_t firstIndex, uint32_t indexCount);

    VkDeviceAddress vertexDeviceAddress() const;
    VkDeviceAddress indexDeviceAddress() const;

    VkFormat positionFormat() const;
    uint32_t vertexStride() const;
    VkDeviceSize positionOffset() const;

    uint32_t firstIndex() const;
    uint32_t indexCount() const;
    uint32_t triangleCount() const;

private:
    geo::MeshGeneric<MeshT>* _mesh;
    uint32_t _firstIndex;
    uint32_t _indexCount;
};

}
}
}
}

