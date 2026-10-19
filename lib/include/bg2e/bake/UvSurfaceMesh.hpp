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
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/math/base.hpp>
#include <vector>
#include <cstddef>

namespace bg2e::bake {
// Explicit native storage-buffer ABI; deindexed triangles, no vertex attributes.
struct alignas(16) UvSurfaceVertex {
    glm::vec4 position{0}, normal{0}, uv{0};
    glm::uvec4 identity{0}; // zero-based submesh, one-based chart, reserved.
};
static_assert(sizeof(UvSurfaceVertex) == 64);
static_assert(offsetof(UvSurfaceVertex,normal) == 16 && offsetof(UvSurfaceVertex,uv) == 32 &&
    offsetof(UvSurfaceVertex,identity) == 48);
struct UvSurfaceMesh {
    std::vector<UvSurfaceVertex> vertices;
    std::vector<geo::Submesh> ranges; // firstIndex/indexCount are firstVertex/vertexCount.
    uint32_t chartCount = 0;
};
// Pure CPU validation/packing of texCoord1. UV overlaps produce a warning and
// remain drawable; malformed ranges/coordinates are rejected. Zero-area UV faces
// cover nothing and are skipped.
// Charts join shared source vertex-index edges within a submesh, not across
// duplicated seam vertices/material/transform boundaries. This is conservative.
BG2E_API UvSurfaceMesh buildUvSurfaceMesh(const geo::Mesh& mesh);
}
