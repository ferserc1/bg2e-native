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

#include <bg2e/geo/GenerateUv2AtlasModifier.hpp>

#include <xatlas.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

namespace bg2e::geo {

namespace {

struct AtlasDeleter
{
    void operator()(xatlas::Atlas * atlas) const
    {
        xatlas::Destroy(atlas);
    }
};
using AtlasPtr = std::unique_ptr<xatlas::Atlas, AtlasDeleter>;

void validateInputMesh(const Mesh & mesh)
{
    if (mesh.vertices.empty())
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: the mesh has no vertices");
    }
    if (mesh.indices.empty() || mesh.indices.size() % 3 != 0)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: index count must be nonzero and divisible by 3");
    }
    if (mesh.submeshes.empty())
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: the mesh has no submeshes");
    }

    const uint32_t vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    for (auto index : mesh.indices)
    {
        if (index >= vertexCount)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: index out of range");
        }
    }

    // Submesh ranges must be ordered, non-overlapping and cover the whole index buffer
    uint32_t expectedFirst = 0;
    for (const auto & submesh : mesh.submeshes)
    {
        if (submesh.indexCount == 0 || submesh.indexCount % 3 != 0)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: submesh index count must be nonzero and divisible by 3");
        }
        if (submesh.firstIndex != expectedFirst)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: submesh ranges must be ordered, non-overlapping and cover the whole index buffer");
        }
        uint64_t end = static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount;
        if (end > mesh.indices.size())
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: submesh range out of bounds");
        }
        expectedFirst += submesh.indexCount;
    }
    if (expectedFirst != mesh.indices.size())
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: submesh ranges do not cover the whole index buffer");
    }
}

// Rasterize each output triangle at atlas resolution and verify that texels
// covered by one chart are not covered by any other chart.
void checkChartSeparation(const xatlas::Atlas * atlas)
{
    const uint32_t width = atlas->width;
    const uint32_t height = atlas->height;
    const uint32_t kEmpty = std::numeric_limits<uint32_t>::max();
    std::vector<uint32_t> grid(static_cast<size_t>(width) * height, kEmpty);

    for (uint32_t m = 0; m < atlas->meshCount; ++m)
    {
        const xatlas::Mesh & outMesh = atlas->meshes[m];
        for (uint32_t t = 0; t + 2 < outMesh.indexCount; t += 3)
        {
            const xatlas::Vertex & v0 = outMesh.vertexArray[outMesh.indexArray[t]];
            const xatlas::Vertex & v1 = outMesh.vertexArray[outMesh.indexArray[t + 1]];
            const xatlas::Vertex & v2 = outMesh.vertexArray[outMesh.indexArray[t + 2]];

            if (v0.chartIndex < 0 || v0.chartIndex != v1.chartIndex || v0.chartIndex != v2.chartIndex)
            {
                throw std::runtime_error("GenerateUv2AtlasModifier: invalid chart assignment in xatlas output");
            }
            const uint32_t chartId = static_cast<uint32_t>(v0.chartIndex);

            const float x0 = v0.uv[0], y0 = v0.uv[1];
            const float x1 = v1.uv[0], y1 = v1.uv[1];
            const float x2 = v2.uv[0], y2 = v2.uv[1];

            const float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
            if (std::abs(area) < 1e-12f)
            {
                continue;
            }

            const int minX = std::max(0, static_cast<int>(std::floor(std::min({ x0, x1, x2 }))));
            const int minY = std::max(0, static_cast<int>(std::floor(std::min({ y0, y1, y2 }))));
            const int maxX = std::min(static_cast<int>(width) - 1, static_cast<int>(std::ceil(std::max({ x0, x1, x2 }))));
            const int maxY = std::min(static_cast<int>(height) - 1, static_cast<int>(std::ceil(std::max({ y0, y1, y2 }))));

            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    const float px = static_cast<float>(x) + 0.5f;
                    const float py = static_cast<float>(y) + 0.5f;
                    const float w0 = (x1 - px) * (y2 - py) - (x2 - px) * (y1 - py);
                    const float w1 = (x2 - px) * (y0 - py) - (x0 - px) * (y2 - py);
                    const float w2 = (x0 - px) * (y1 - py) - (x1 - px) * (y0 - py);
                    const bool inside = area > 0.0f
                        ? (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                        : (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);
                    if (!inside)
                    {
                        continue;
                    }

                    auto & cell = grid[static_cast<size_t>(y) * width + x];
                    if (cell == kEmpty)
                    {
                        cell = chartId;
                    }
                    else if (cell != chartId)
                    {
                        throw std::runtime_error(
                            "GenerateUv2AtlasModifier: atlas charts overlap; increase Uv2AtlasOptions::paddingPixels");
                    }
                }
            }
        }
    }
}

} // anonymous namespace

void GenerateUv2AtlasModifier::apply()
{
    if (!_mesh)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: null mesh");
    }
    if (_options.resolution == 0)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: resolution must be greater than zero");
    }

    validateInputMesh(*_mesh);

    const uint32_t sourceVertexCount = static_cast<uint32_t>(_mesh->vertices.size());
    AtlasPtr atlas(xatlas::Create());
    if (!atlas)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: xatlas::Create failed");
    }

    // A submesh is a material/index range, not a separate object for UV
    // generation. Submit the whole mesh so chart generation can see every
    // triangle before all charts are packed into the one atlas.
    xatlas::MeshDecl decl;
    decl.vertexCount = sourceVertexCount;
    decl.vertexPositionData = &_mesh->vertices[0].position;
    decl.vertexPositionStride = sizeof(VertexPNUUT);
    decl.vertexNormalData = &_mesh->vertices[0].normal;
    decl.vertexNormalStride = sizeof(VertexPNUUT);
    decl.indexData = _mesh->indices.data();
    decl.indexCount = static_cast<uint32_t>(_mesh->indices.size());
    decl.faceCount = decl.indexCount / 3;
    decl.indexFormat = xatlas::IndexFormat::UInt32;

    const auto error = xatlas::AddMesh(atlas.get(), decl);
    if (error != xatlas::AddMeshError::Success)
    {
        throw std::runtime_error(std::string("GenerateUv2AtlasModifier: xatlas::AddMesh failed: ") +
            xatlas::StringForEnum(error));
    }

    xatlas::ChartOptions chartOptions;
    xatlas::PackOptions packOptions;
    packOptions.resolution = _options.resolution;
    packOptions.padding = _options.paddingPixels;
    packOptions.bilinear = true;
    packOptions.blockAlign = false;
    packOptions.createImage = false;
    xatlas::Generate(atlas.get(), chartOptions, packOptions);

    if (atlas->meshCount != 1)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: unexpected xatlas output mesh count");
    }
    if (atlas->atlasCount != 1)
    {
        throw std::runtime_error(
            "GenerateUv2AtlasModifier: xatlas produced " + std::to_string(atlas->atlasCount) +
            " atlases; exactly one is required (increase resolution or reduce padding)");
    }
    if (atlas->width == 0 || atlas->height == 0)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: xatlas produced an empty atlas");
    }

    // Build fresh arrays; map every output vertex through xref to the source
    // vertex and copy position, normal, tangent and UV1 exactly.
    std::vector<VertexPNUUT> newVertices;
    std::vector<uint32_t> newIndices;
    std::vector<Submesh> newSubmeshes = _mesh->submeshes;

    const float invWidth = 1.0f / static_cast<float>(atlas->width);
    const float invHeight = 1.0f / static_cast<float>(atlas->height);

    const xatlas::Mesh & outMesh = atlas->meshes[0];
    if (outMesh.indexCount != _mesh->indices.size() || outMesh.indexCount % 3 != 0)
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: xatlas output triangle count mismatch");
    }

    newVertices.reserve(outMesh.vertexCount);
    for (uint32_t v = 0; v < outMesh.vertexCount; ++v)
    {
        const xatlas::Vertex & outVertex = outMesh.vertexArray[v];
        if (outVertex.xref >= sourceVertexCount)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: xatlas output vertex xref out of range");
        }
        if (outVertex.atlasIndex != 0 || outVertex.chartIndex < 0)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: xatlas output vertex is not in the single atlas");
        }
        if (!std::isfinite(outVertex.uv[0]) || !std::isfinite(outVertex.uv[1]))
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: xatlas produced non-finite UV coordinates");
        }

        float u = outVertex.uv[0] * invWidth;
        float vcoord = outVertex.uv[1] * invHeight;
        constexpr float kEpsilon = 1e-4f;
        if (u < -kEpsilon || u > 1.0f + kEpsilon || vcoord < -kEpsilon || vcoord > 1.0f + kEpsilon)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: xatlas produced out-of-range UV coordinates");
        }
        u = std::clamp(u, 0.0f, 1.0f);
        vcoord = std::clamp(vcoord, 0.0f, 1.0f);

        VertexPNUUT newVertex = _mesh->vertices[outVertex.xref];
        newVertex.texCoord1 = { u, vcoord };
        newVertices.push_back(newVertex);
    }

    // xatlas writes each output triangle into its original input face slot.
    // Keeping that index order also keeps every original submesh range and
    // therefore its material association.
    newIndices.reserve(outMesh.indexCount);
    for (uint32_t i = 0; i < outMesh.indexCount; ++i)
    {
        const uint32_t index = outMesh.indexArray[i];
        if (index >= outMesh.vertexCount)
        {
            throw std::runtime_error("GenerateUv2AtlasModifier: xatlas output index out of range");
        }
        newIndices.push_back(index);
    }

    if (newIndices.size() != _mesh->indices.size())
    {
        throw std::runtime_error("GenerateUv2AtlasModifier: output triangle count does not match the input mesh");
    }

    checkChartSeparation(atlas.get());

    Uv2AtlasResult newResult;
    newResult.width = atlas->width;
    newResult.height = atlas->height;
    newResult.chartCount = atlas->chartCount;
    newResult.utilization = atlas->utilization ? atlas->utilization[0] : 0.0f;

    // Commit only after every validation succeeded
    _mesh->vertices = std::move(newVertices);
    _mesh->indices = std::move(newIndices);
    _mesh->submeshes = std::move(newSubmeshes);
    _result = newResult;
}

} // namespace bg2e::geo
