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

#include <bg2e/geo/UvAtlasValidator.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace bg2e::geo {

namespace {

constexpr double kMinTriangleArea = 1.0e-12;
constexpr double kOverlapAreaEpsilon = 1.0e-12;
constexpr double kBoundsEpsilon = 1.0e-12;
constexpr uint32_t kCoverageGridSize = 512;

struct UvPoint
{
    double x;
    double y;
};

struct UvTriangle
{
    std::array<UvPoint, 3> points;
    double minX;
    double maxX;
    double minY;
    double maxY;
    uint32_t submesh;
    uint32_t triangle;
};

double cross(const UvPoint & a, const UvPoint & b, const UvPoint & c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double polygonArea(const std::vector<UvPoint> & polygon)
{
    if (polygon.size() < 3)
    {
        return 0.0;
    }
    double twiceArea = 0.0;
    for (size_t index = 0; index < polygon.size(); ++index)
    {
        const auto & current = polygon[index];
        const auto & next = polygon[(index + 1) % polygon.size()];
        twiceArea += current.x * next.y - current.y * next.x;
    }
    return std::abs(twiceArea) * 0.5;
}

// Sutherland-Hodgman clip of the first triangle against the second one.
// Shared edges/vertices yield a zero-area intersection and are allowed.
double triangleIntersectionArea(const UvTriangle & first, const UvTriangle & second)
{
    std::vector<UvPoint> polygon(first.points.begin(), first.points.end());
    const double clipOrientation = cross(
        second.points[0], second.points[1], second.points[2]) >= 0.0 ? 1.0 : -1.0;

    for (size_t edge = 0; edge < 3 && !polygon.empty(); ++edge)
    {
        const UvPoint & edgeStart = second.points[edge];
        const UvPoint & edgeEnd = second.points[(edge + 1) % 3];
        std::vector<UvPoint> clipped;
        clipped.reserve(polygon.size() + 1);

        UvPoint previous = polygon.back();
        double previousDistance = clipOrientation * cross(edgeStart, edgeEnd, previous);
        bool previousInside = previousDistance >= -kBoundsEpsilon;
        for (const auto & current : polygon)
        {
            const double currentDistance = clipOrientation * cross(edgeStart, edgeEnd, current);
            const bool currentInside = currentDistance >= -kBoundsEpsilon;
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

UvAtlasValidation fail(UvAtlasError error, std::string message)
{
    UvAtlasValidation result;
    result.valid = false;
    result.error = error;
    result.message = std::move(message);
    return result;
}

float computeCoverage(const std::vector<UvTriangle> & triangles)
{
    std::vector<uint8_t> grid(static_cast<size_t>(kCoverageGridSize) * kCoverageGridSize, 0);
    for (const auto & triangle : triangles)
    {
        const auto x0 = static_cast<float>(triangle.points[0].x) * kCoverageGridSize;
        const auto y0 = static_cast<float>(triangle.points[0].y) * kCoverageGridSize;
        const auto x1 = static_cast<float>(triangle.points[1].x) * kCoverageGridSize;
        const auto y1 = static_cast<float>(triangle.points[1].y) * kCoverageGridSize;
        const auto x2 = static_cast<float>(triangle.points[2].x) * kCoverageGridSize;
        const auto y2 = static_cast<float>(triangle.points[2].y) * kCoverageGridSize;

        const float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
        if (std::abs(area) < 1e-12f)
        {
            continue;
        }

        const int minX = std::max(0, static_cast<int>(std::floor(std::min({ x0, x1, x2 }))));
        const int minY = std::max(0, static_cast<int>(std::floor(std::min({ y0, y1, y2 }))));
        const int maxX = std::min(static_cast<int>(kCoverageGridSize) - 1,
            static_cast<int>(std::ceil(std::max({ x0, x1, x2 }))));
        const int maxY = std::min(static_cast<int>(kCoverageGridSize) - 1,
            static_cast<int>(std::ceil(std::max({ y0, y1, y2 }))));

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
                if (inside)
                {
                    grid[static_cast<size_t>(y) * kCoverageGridSize + x] = 1;
                }
            }
        }
    }

    size_t covered = 0;
    for (auto cell : grid)
    {
        covered += cell;
    }
    return static_cast<float>(covered) / static_cast<float>(grid.size());
}

} // anonymous namespace

UvAtlasValidation UvAtlasValidator::validate(const Mesh & mesh, uint32_t uvSet)
{
    if (uvSet > 1)
    {
        return fail(UvAtlasError::UnsupportedUvSet,
            "unsupported UV set " + std::to_string(uvSet) + "; only 0 (UV1) and 1 (UV2) exist");
    }
    const auto uvOf = [uvSet](const VertexPNUUT & vertex) -> const glm::vec2 & {
        return uvSet == 0 ? vertex.texCoord0 : vertex.texCoord1;
    };

    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        return fail(UvAtlasError::EmptyMesh, "the mesh has no vertices or no indices");
    }
    if (mesh.indices.size() % 3 != 0)
    {
        return fail(UvAtlasError::InvalidSubmeshRange, "index count is not divisible by 3");
    }
    if (mesh.submeshes.empty())
    {
        return fail(UvAtlasError::MissingSubmeshes, "the mesh has no submeshes");
    }

    // Submesh ranges must be in bounds, triangle aligned, non-overlapping and
    // cover the whole index buffer.
    uint64_t totalSubmeshIndices = 0;
    std::vector<std::pair<uint32_t, uint32_t>> ranges;
    ranges.reserve(mesh.submeshes.size());
    for (uint32_t submeshIndex = 0; submeshIndex < mesh.submeshes.size(); ++submeshIndex)
    {
        const auto & submesh = mesh.submeshes[submeshIndex];
        const uint64_t rangeEnd = static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount;
        if (submesh.indexCount == 0 || submesh.indexCount % 3 != 0 || rangeEnd > mesh.indices.size())
        {
            auto result = fail(UvAtlasError::InvalidSubmeshRange,
                "invalid index range in submesh " + std::to_string(submeshIndex));
            result.submeshIndex = submeshIndex;
            return result;
        }
        totalSubmeshIndices += submesh.indexCount;
        ranges.emplace_back(submesh.firstIndex, submesh.indexCount);
    }
    auto ordered = ranges;
    std::sort(ordered.begin(), ordered.end());
    for (size_t i = 1; i < ordered.size(); ++i)
    {
        if (ordered[i].first < static_cast<uint64_t>(ordered[i - 1].first) + ordered[i - 1].second)
        {
            return fail(UvAtlasError::OverlappingSubmeshRanges, "submesh index ranges overlap");
        }
    }
    if (totalSubmeshIndices != mesh.indices.size())
    {
        return fail(UvAtlasError::IncompleteSubmeshCoverage,
            "submesh ranges do not cover the whole index buffer");
    }

    for (uint32_t vertexIndex = 0; vertexIndex < mesh.vertices.size(); ++vertexIndex)
    {
        const auto & uv = uvOf(mesh.vertices[vertexIndex]);
        if (!std::isfinite(uv.x) || !std::isfinite(uv.y))
        {
            auto result = fail(UvAtlasError::NonFiniteCoordinate,
                "non-finite coordinate at vertex " + std::to_string(vertexIndex));
            result.vertexIndex = vertexIndex;
            return result;
        }
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        {
            auto result = fail(UvAtlasError::CoordinateOutOfRange,
                "coordinate outside [0,1] at vertex " + std::to_string(vertexIndex));
            result.vertexIndex = vertexIndex;
            return result;
        }
    }

    UvAtlasValidation result;
    result.error = UvAtlasError::None;

    std::vector<UvTriangle> triangles;
    double mappedArea = 0.0;
    for (uint32_t submeshIndex = 0; submeshIndex < mesh.submeshes.size(); ++submeshIndex)
    {
        const auto & submesh = mesh.submeshes[submeshIndex];
        const uint64_t rangeEnd = static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount;
        for (uint64_t index = submesh.firstIndex; index < rangeEnd; index += 3)
        {
            const uint32_t triangleIndex = static_cast<uint32_t>((index - submesh.firstIndex) / 3);
            UvTriangle triangle{};
            triangle.submesh = submeshIndex;
            triangle.triangle = triangleIndex;
            for (uint32_t corner = 0; corner < 3; ++corner)
            {
                const uint32_t vertexIndex = mesh.indices[static_cast<size_t>(index + corner)];
                if (vertexIndex >= mesh.vertices.size())
                {
                    auto error = fail(UvAtlasError::IndexOutOfRange,
                        "out-of-range vertex index in submesh " + std::to_string(submeshIndex));
                    error.submeshIndex = submeshIndex;
                    error.triangleIndex = triangleIndex;
                    return error;
                }
                const auto & uv = uvOf(mesh.vertices[vertexIndex]);
                triangle.points[corner] = { uv.x, uv.y };
            }

            const double signedArea = cross(triangle.points[0], triangle.points[1], triangle.points[2]) * 0.5;
            if (std::abs(signedArea) <= kMinTriangleArea)
            {
                auto error = fail(UvAtlasError::DegenerateTriangle,
                    "zero-area or near-degenerate mapped triangle in submesh " +
                    std::to_string(submeshIndex));
                error.submeshIndex = submeshIndex;
                error.triangleIndex = triangleIndex;
                return error;
            }
            mappedArea += std::abs(signedArea);

            triangle.minX = std::min({ triangle.points[0].x, triangle.points[1].x, triangle.points[2].x });
            triangle.maxX = std::max({ triangle.points[0].x, triangle.points[1].x, triangle.points[2].x });
            triangle.minY = std::min({ triangle.points[0].y, triangle.points[1].y, triangle.points[2].y });
            triangle.maxY = std::max({ triangle.points[0].y, triangle.points[1].y, triangle.points[2].y });
            triangles.push_back(triangle);
        }
    }

    std::sort(triangles.begin(), triangles.end(), [](const UvTriangle & lhs, const UvTriangle & rhs) {
        return lhs.minX < rhs.minX;
    });
    for (size_t firstIndex = 0; firstIndex < triangles.size(); ++firstIndex)
    {
        const auto & first = triangles[firstIndex];
        for (size_t secondIndex = firstIndex + 1; secondIndex < triangles.size(); ++secondIndex)
        {
            const auto & second = triangles[secondIndex];
            if (second.minX >= first.maxX - kBoundsEpsilon)
            {
                break;
            }
            if (second.maxY <= first.minY + kBoundsEpsilon ||
                second.minY >= first.maxY - kBoundsEpsilon)
            {
                continue;
            }
            if (triangleIntersectionArea(first, second) > kOverlapAreaEpsilon)
            {
                auto error = fail(UvAtlasError::OverlappingTriangles,
                    "overlapping positive-area triangles in submeshes " +
                    std::to_string(std::min(first.submesh, second.submesh)) + " and " +
                    std::to_string(std::max(first.submesh, second.submesh)));
                error.submeshIndex = std::min(first.submesh, second.submesh);
                error.triangleIndex = first.triangle;
                return error;
            }
        }
    }

    result.valid = true;
    result.triangleCount = static_cast<uint32_t>(triangles.size());
    result.mappedArea = static_cast<float>(mappedArea);
    result.coverage = computeCoverage(triangles);
    return result;
}

} // namespace bg2e::geo
