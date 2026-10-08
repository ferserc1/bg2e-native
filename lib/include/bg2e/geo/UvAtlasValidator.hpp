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
#include <bg2e/geo/Mesh.hpp>

#include <cstdint>
#include <string>

namespace bg2e {
namespace geo {

enum class UvAtlasError
{
    None,
    UnsupportedUvSet,
    EmptyMesh,
    MissingSubmeshes,
    InvalidSubmeshRange,
    OverlappingSubmeshRanges,
    IncompleteSubmeshCoverage,
    IndexOutOfRange,
    NonFiniteCoordinate,
    CoordinateOutOfRange,
    DegenerateTriangle,
    OverlappingTriangles
};

struct UvAtlasValidation
{
    bool valid = false;
    UvAtlasError error = UvAtlasError::None;
    std::string message;
    uint32_t submeshIndex = 0;   // Submesh related to the error (lowest one for overlaps)
    uint32_t triangleIndex = 0;  // Triangle index within the submesh
    uint32_t vertexIndex = 0;    // Vertex index for coordinate errors
    uint32_t triangleCount = 0;  // Total validated triangles
    float mappedArea = 0.0f;     // Sum of absolute UV triangle areas; may exceed 1 when triangles overlap
    float coverage = 0.0f;       // Unique fraction of the unit square covered by the UV map
};

// Usable-atlas test for a standard mesh UV set (0 = UV1 / texCoord0,
// 1 = UV2 / texCoord1). Rejects non-finite or out-of-range coordinates,
// overlapping positive-area triangles and invalid index/submesh ranges.
// Zero-area mapped triangles cover no texels and are ignored by the overlap
// and coverage checks; a map without any positive-area triangle is rejected.
// Shared edges and vertices are allowed.
// A UV1 copy already satisfying every atlas rule is a valid atlas;
// coordinate equality with another UV set is never used to infer provenance.
class BG2E_API UvAtlasValidator
{
public:
    static UvAtlasValidation validate(const Mesh & mesh, uint32_t uvSet);
};

}
}
