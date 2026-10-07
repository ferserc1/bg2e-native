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
#include <bg2e/geo/modifiers.hpp>

namespace bg2e {
namespace geo {

struct Uv2AtlasOptions
{
    uint32_t resolution = 512;
    uint32_t paddingPixels = 4;
};

struct Uv2AtlasResult
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t chartCount = 0;
    float utilization = 0.0f;
};

// Generates a new UV2 atlas for a standard Mesh (VertexPNUUT) using xatlas.
// The complete Mesh is fed into xatlas as one input mesh and one atlas,
// preserving submesh order, triangle count and material-to-submesh association.
// Previous UV2 values are ignored. Position, normal,
// tangent and UV1 (texCoord0) are copied bit-for-bit from the source vertex
// referenced by each output vertex; only UV2 (texCoord1) is replaced.
//
// This modifier is CPU-only: it neither loads nor reloads GPU resources.
// On failure apply() throws std::runtime_error and leaves the mesh unchanged.
class BG2E_API GenerateUv2AtlasModifier final : public Modifier<Mesh> {
public:
    GenerateUv2AtlasModifier()
        :Modifier<Mesh>()
    {}

    GenerateUv2AtlasModifier(Mesh * mesh, const Uv2AtlasOptions & options = {})
        :Modifier<Mesh>(mesh)
        ,_options { options }
    {}

    inline void setOptions(const Uv2AtlasOptions & options) { _options = options; }
    inline const Uv2AtlasOptions & options() const { return _options; }

    void apply() override;

    // Valid only after a successful apply()
    inline const Uv2AtlasResult & result() const { return _result; }

protected:
    Uv2AtlasOptions _options;
    Uv2AtlasResult _result;
};

}
}
