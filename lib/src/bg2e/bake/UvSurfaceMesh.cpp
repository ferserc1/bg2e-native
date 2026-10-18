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

#include <bg2e/bake/UvSurfaceMesh.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <array>
#include <utility>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>

namespace bg2e::bake {
namespace {
using Edge = std::array<uint32_t,3>; // submesh and sorted source vertex indices.

bool finite(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
}

UvSurfaceMesh buildUvSurfaceMesh(const geo::Mesh& mesh)
{
    if (mesh.indices.size() > std::numeric_limits<uint32_t>::max() ||
        mesh.submeshes.size() >= std::numeric_limits<uint32_t>::max())
    {
        throw std::length_error("UV surface vertex/submesh identity exceeds uint32");
    }
    const auto validation = geo::UvAtlasValidator::validate(mesh,1);
    if (!validation.valid)
    {
        throw std::invalid_argument("Invalid bake UV2 atlas: "+validation.message);
    }
    const auto triangles = mesh.indices.size()/3;
    std::vector<uint32_t> parent(triangles);
    std::iota(parent.begin(),parent.end(),0u);
    auto root = [&](uint32_t id)
    {
        while (parent[id] != id)
        {
            parent[id] = parent[parent[id]];
            id = parent[id];
        }
        return id;
    };
    std::vector<bool> active(triangles,false);
    std::map<Edge,uint32_t> edges;
    for (uint32_t submesh = 0; submesh < mesh.submeshes.size(); ++submesh)
    {
        const auto range = mesh.submeshes[submesh];
        for (uint32_t offset = 0; offset < range.indexCount; offset += 3)
        {
            const uint32_t first = range.firstIndex+offset, triangle = first/3;
            const geo::Vertex* v[] = {&mesh.vertices[mesh.indices[first]],
                &mesh.vertices[mesh.indices[first+1]],&mesh.vertices[mesh.indices[first+2]]};
            const auto a = glm::dvec2(v[1]->texCoord1)-glm::dvec2(v[0]->texCoord1);
            const auto b = glm::dvec2(v[2]->texCoord1)-glm::dvec2(v[0]->texCoord1);
            if (std::abs(a.x*b.y-a.y*b.x)*0.5 <= 1e-12)
            {
                continue;
            }
            const auto edgeA = glm::dvec3(v[1]->position)-glm::dvec3(v[0]->position);
            const auto edgeB = glm::dvec3(v[2]->position)-glm::dvec3(v[0]->position);
            const auto area = glm::length(glm::cross(edgeA,edgeB));
            if (!std::isfinite(area) || area == 0)
            {
                throw std::invalid_argument("Positive UV area cannot map degenerate/nonfinite geometry");
            }
            active[triangle] = true;
            for (auto* vertex : v)
            {
                const float length = glm::length(vertex->normal);
                if (!finite(vertex->position) || !finite(vertex->normal) || !std::isfinite(length) || length <= 1e-12f)
                {
                    throw std::invalid_argument("UV surface needs finite positions and nonzero finite normals");
                }
            }
            for (size_t edge = 0; edge < 3; ++edge)
            {
                auto start = mesh.indices[first+edge], end = mesh.indices[first+(edge+1)%3];
                if (end < start)
                {
                    std::swap(start,end);
                }
                const Edge key{submesh,start,end};
                auto [found,inserted] = edges.emplace(key,triangle);
                if (!inserted)
                {
                    parent[root(triangle)] = root(found->second);
                }
            }
        }
    }
    UvSurfaceMesh result;
    std::map<uint32_t,uint32_t> charts;
    result.vertices.reserve(mesh.indices.size());
    for (uint32_t submesh = 0; submesh < mesh.submeshes.size(); ++submesh)
    {
        const auto range = mesh.submeshes[submesh];
        const auto firstVertex = uint32_t(result.vertices.size());
        for (uint32_t offset = 0; offset < range.indexCount; offset += 3)
        {
            const auto first = range.firstIndex+offset;
            if (!active[first/3])
            {
                continue;
            }
            const auto chart = charts.emplace(root(first/3),uint32_t(charts.size()+1)).first;
            for (uint32_t corner = 0; corner < 3; ++corner)
            {
                const auto& vertex = mesh.vertices[mesh.indices[first+corner]];
                result.vertices.push_back({glm::vec4(vertex.position,1),glm::vec4(vertex.normal,0),
                    glm::vec4(vertex.texCoord1,0,0),glm::uvec4(submesh,chart->second,0,0)});
            }
        }
        result.ranges.push_back({firstVertex,uint32_t(result.vertices.size())-firstVertex});
    }
    result.chartCount = uint32_t(charts.size());
    if (result.vertices.empty())
    {
        throw std::invalid_argument("UV atlas has no drawable surface triangles");
    }
    return result;
}
}
