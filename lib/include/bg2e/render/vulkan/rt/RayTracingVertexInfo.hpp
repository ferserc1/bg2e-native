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
#include <bg2e/render/Engine.hpp>
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/geo/Vertex.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

template <typename MeshT>
struct RayTracingVertexInfo;

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshP> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexP);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexP, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPN> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPN);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPN, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPC> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPC);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPC, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPU> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPU);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPU, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPNU> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPNU);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPNU, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPNC> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPNC);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPNC, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPNUC> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPNUC);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPNUC, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPNUT> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPNUT);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPNUT, position);
};

template <>
struct RayTracingVertexInfo<bg2e::geo::MeshPNUUT> {
    static constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr size_t kStride = sizeof(bg2e::geo::VertexPNUUT);
    static constexpr VkDeviceSize kPositionOffset = offsetof(bg2e::geo::VertexPNUUT, position);
};

}
}
}
}

