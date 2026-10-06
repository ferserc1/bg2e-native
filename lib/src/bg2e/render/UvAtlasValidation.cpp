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

#include "UvAtlasValidation.hpp"

#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/Mesh.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace bg2e {
namespace render {
namespace detail {

namespace {

struct UvPoint {
    double x;
    double y;
};

struct UvTriangle {
    std::array<UvPoint, 3> points;
    double minX;
    double maxX;
    double minY;
    double maxY;
    uint32_t submesh;
};

double cross(const UvPoint& a, const UvPoint& b, const UvPoint& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double polygonArea(const std::vector<UvPoint>& polygon)
{
    if (polygon.size() < 3)
    {
        return 0.0;
    }
    double twiceArea = 0.0;
    for (size_t index = 0; index < polygon.size(); ++index)
    {
        const auto& current = polygon[index];
        const auto& next = polygon[(index + 1) % polygon.size()];
        twiceArea += current.x * next.y - current.y * next.x;
    }
    return std::abs(twiceArea) * 0.5;
}

double triangleIntersectionArea(const UvTriangle& first, const UvTriangle& second)
{
    constexpr double edgeEpsilon = 1.0e-12;
    std::vector<UvPoint> polygon(first.points.begin(), first.points.end());
    const double clipOrientation = cross(
        second.points[0], second.points[1], second.points[2]) >= 0.0 ? 1.0 : -1.0;

    for (size_t edge = 0; edge < 3 && !polygon.empty(); ++edge)
    {
        const UvPoint& edgeStart = second.points[edge];
        const UvPoint& edgeEnd = second.points[(edge + 1) % 3];
        std::vector<UvPoint> clipped;
        clipped.reserve(polygon.size() + 1);

        UvPoint previous = polygon.back();
        double previousDistance = clipOrientation * cross(edgeStart, edgeEnd, previous);
        bool previousInside = previousDistance >= -edgeEpsilon;
        for (const UvPoint& current : polygon)
        {
            const double currentDistance = clipOrientation * cross(edgeStart, edgeEnd, current);
            const bool currentInside = currentDistance >= -edgeEpsilon;
            if (currentInside != previousInside)
            {
                const double denominator = previousDistance - currentDistance;
                if (std::abs(denominator) > std::numeric_limits<double>::epsilon())
                {
                    const double t = previousDistance / denominator;
                    clipped.push_back({
                        previous.x + (current.x - previous.x) * t,
                        previous.y + (current.y - previous.y) * t
                    });
                }
            }
            if (currentInside)
            {
                clipped.push_back(current);
            }
            previous = current;
            previousDistance = currentDistance;
            previousInside = currentInside;
        }
        polygon = std::move(clipped);
    }
    return polygonArea(polygon);
}

}

bool validateUsableUv2(const scene::Drawable& drawable, std::string& error)
{
    constexpr double minTriangleArea = 1.0e-12;
    constexpr double overlapAreaEpsilon = 1.0e-12;
    constexpr double boundsEpsilon = 1.0e-12;

    const auto mesh = drawable.mesh();
    if (!mesh)
    {
        error = "UV2 atlas is missing its CPU mesh";
        return false;
    }
    if (mesh->submeshes.size() != drawable.submeshesCount())
    {
        error = "UV2 atlas submesh metadata does not match its CPU mesh";
        return false;
    }

    for (size_t vertexIndex = 0; vertexIndex < mesh->vertices.size(); ++vertexIndex)
    {
        const auto& uv = mesh->vertices[vertexIndex].texCoord1;
        if (!std::isfinite(uv.x) || !std::isfinite(uv.y))
        {
            error = "UV2 atlas contains a non-finite coordinate at vertex " + std::to_string(vertexIndex);
            return false;
        }
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        {
            error = "UV2 atlas contains a coordinate outside [0,1] at vertex " + std::to_string(vertexIndex);
            return false;
        }
    }

    std::vector<UvTriangle> triangles;
    for (uint32_t submeshIndex = 0; submeshIndex < drawable.submeshesCount(); ++submeshIndex)
    {
        const auto submesh = drawable.submeshData(submeshIndex);
        const uint64_t rangeEnd = static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount;
        if (submesh.indexCount == 0 || submesh.indexCount % 3 != 0 || rangeEnd > mesh->indices.size())
        {
            error = "UV2 atlas has an invalid index range in submesh " + std::to_string(submeshIndex);
            return false;
        }

        for (uint64_t index = submesh.firstIndex; index < rangeEnd; index += 3)
        {
            UvTriangle triangle{};
            triangle.submesh = submeshIndex;
            for (uint32_t corner = 0; corner < 3; ++corner)
            {
                const uint32_t vertexIndex = mesh->indices[static_cast<size_t>(index + corner)];
                if (vertexIndex >= mesh->vertices.size())
                {
                    error = "UV2 atlas has an out-of-range vertex index in submesh " +
                        std::to_string(submeshIndex);
                    return false;
                }
                const auto& uv = mesh->vertices[vertexIndex].texCoord1;
                triangle.points[corner] = { uv.x, uv.y };
            }

            const double signedArea = cross(triangle.points[0], triangle.points[1], triangle.points[2]) * 0.5;
            if (std::abs(signedArea) <= minTriangleArea)
            {
                error = "UV2 atlas has a zero-area or near-degenerate triangle in submesh " +
                    std::to_string(submeshIndex);
                return false;
            }
            triangle.minX = std::min({ triangle.points[0].x, triangle.points[1].x, triangle.points[2].x });
            triangle.maxX = std::max({ triangle.points[0].x, triangle.points[1].x, triangle.points[2].x });
            triangle.minY = std::min({ triangle.points[0].y, triangle.points[1].y, triangle.points[2].y });
            triangle.maxY = std::max({ triangle.points[0].y, triangle.points[1].y, triangle.points[2].y });
            triangles.push_back(triangle);
        }
    }

    std::sort(triangles.begin(), triangles.end(), [](const UvTriangle& lhs, const UvTriangle& rhs) {
        return lhs.minX < rhs.minX;
    });
    for (size_t firstIndex = 0; firstIndex < triangles.size(); ++firstIndex)
    {
        const auto& first = triangles[firstIndex];
        for (size_t secondIndex = firstIndex + 1; secondIndex < triangles.size(); ++secondIndex)
        {
            const auto& second = triangles[secondIndex];
            if (second.minX >= first.maxX - boundsEpsilon)
            {
                break;
            }
            if (second.maxY <= first.minY + boundsEpsilon ||
                second.minY >= first.maxY - boundsEpsilon)
            {
                continue;
            }
            if (triangleIntersectionArea(first, second) > overlapAreaEpsilon)
            {
                error = "UV2 atlas has overlapping positive-area triangles in submeshes " +
                    std::to_string(first.submesh) + " and " + std::to_string(second.submesh);
                return false;
            }
        }
    }

    error.clear();
    return true;
}

}
}
}
