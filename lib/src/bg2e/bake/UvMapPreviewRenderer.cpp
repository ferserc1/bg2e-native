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

#include <bg2e/bake/UvMapPreviewRenderer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace bg2e::bake {
namespace {

using Point = std::array<float, 2>;
using Color = std::array<uint8_t, 4>;

constexpr std::array<Color, 8> kColors{{
    {245, 166, 35, 255}, {66, 196, 150, 255}, {92, 145, 255, 255},
    {226, 95, 113, 255}, {181, 122, 255, 255}, {63, 201, 220, 255},
    {232, 208, 82, 255}, {245, 126, 190, 255}}};

uint8_t mix(uint8_t a, uint8_t b)
{
    return static_cast<uint8_t>((static_cast<uint32_t>(a) * 3u + b) / 4u);
}

} // namespace

UvMapPreviewRenderer::UvMapPreviewRenderer(gpu::Device& device, uint32_t resolution)
    : _device(&device), _resolution(resolution)
{
    if (resolution == 0 || resolution > 16384)
    {
        throw std::invalid_argument("UV preview resolution must be in [1, 16384]");
    }
    createTarget();
}

void UvMapPreviewRenderer::setResolution(uint32_t resolution)
{
    if (resolution == 0 || resolution > 16384)
    {
        throw std::invalid_argument("UV preview resolution must be in [1, 16384]");
    }
    if (_resolution != resolution)
    {
        _device->waitIdle();
        _resolution = resolution;
        createTarget();
    }
}

uint32_t UvMapPreviewRenderer::resolution() const
{
    return _resolution;
}

std::shared_ptr<gpu::Image> UvMapPreviewRenderer::image() const
{
    return _image;
}

const std::vector<uint8_t>& UvMapPreviewRenderer::pixels() const
{
    return _pixels;
}

void UvMapPreviewRenderer::createTarget()
{
    const uint64_t texels = static_cast<uint64_t>(_resolution) * _resolution;
    if (texels > std::numeric_limits<size_t>::max() / 4u)
    {
        throw std::length_error("UV preview resolution overflows host address space");
    }
    const size_t bytes = static_cast<size_t>(texels * 4u);
    _pixels.assign(static_cast<size_t>(bytes), 0);
    gpu::ImageDescription description;
    description.size = {_resolution, _resolution};
    description.format = gpu::PixelFormat::R8G8B8A8_UNORM;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    description.debugName = "Bake UV map preview";
    _image = _device->createImage(description);
}

void UvMapPreviewRenderer::render(const geo::Mesh& mesh, uint32_t uvSet)
{
    if (uvSet > 1)
    {
        throw std::invalid_argument("UV set must be 0 (UV1) or 1 (UV2)");
    }
    constexpr Color background{18, 21, 27, 255};
    std::fill(_pixels.begin(), _pixels.end(), 0);
    for (size_t i = 0; i < _pixels.size(); i += 4)
    {
        std::copy(background.begin(), background.end(), _pixels.begin() + static_cast<std::ptrdiff_t>(i));
    }

    const auto pixel = [this](int x, int y) -> uint8_t*
    {
        if (x < 0 || y < 0 || x >= static_cast<int>(_resolution) || y >= static_cast<int>(_resolution))
        {
            return nullptr;
        }
        return _pixels.data() + (static_cast<size_t>(y) * _resolution + static_cast<size_t>(x)) * 4;
    };
    const auto line = [&](Point a, Point b, Color color)
    {
        const int x0 = static_cast<int>(std::lround(a[0]));
        const int y0 = static_cast<int>(std::lround(a[1]));
        const int x1 = static_cast<int>(std::lround(b[0]));
        const int y1 = static_cast<int>(std::lround(b[1]));
        const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int error = dx + dy, x = x0, y = y0;
        for (;;)
        {
            if (uint8_t* dst = pixel(x, y))
            {
                std::copy(color.begin(), color.end(), dst);
            }
            if (x == x1 && y == y1)
            {
                break;
            }
            const int twice = 2 * error;
            if (twice >= dy)
            {
                error += dy;
                x += sx;
            }
            if (twice <= dx)
            {
                error += dx;
                y += sy;
            }
        }
    };
    const auto drawTriangle = [&](std::array<Point, 3> p, Color color)
    {
        const float area = (p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) -
                           (p[1][1] - p[0][1]) * (p[2][0] - p[0][0]);
        if (std::abs(area) < 1.0e-6f)
        {
            return;
        }
        const int minX = std::max(0, static_cast<int>(std::floor(std::min({p[0][0], p[1][0], p[2][0]}))));
        const int minY = std::max(0, static_cast<int>(std::floor(std::min({p[0][1], p[1][1], p[2][1]}))));
        const int maxX = std::min(static_cast<int>(_resolution) - 1, static_cast<int>(std::ceil(std::max({p[0][0], p[1][0], p[2][0]}))));
        const int maxY = std::min(static_cast<int>(_resolution) - 1, static_cast<int>(std::ceil(std::max({p[0][1], p[1][1], p[2][1]}))));
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                const float px = x + 0.5f, py = y + 0.5f;
                const float w0 = ((p[1][0] - px) * (p[2][1] - py) - (p[1][1] - py) * (p[2][0] - px)) / area;
                const float w1 = ((p[2][0] - px) * (p[0][1] - py) - (p[2][1] - py) * (p[0][0] - px)) / area;
                if (w0 >= 0.0f && w1 >= 0.0f && w0 + w1 <= 1.0f)
                {
                    if (uint8_t* dst = pixel(x, y))
                    {
                        for (size_t c = 0; c < 3; ++c)
                        {
                            dst[c] = mix(dst[c], color[c]);
                        }
                    }
                }
            }
        }
        const Color edge{color[0], color[1], color[2], 255};
        line(p[0], p[1], edge);
        line(p[1], p[2], edge);
        line(p[2], p[0], edge);
    };
    const auto uv = [uvSet](const geo::VertexPNUUT& vertex)
    {
        return uvSet == 0 ? vertex.texCoord0 : vertex.texCoord1;
    };
    const auto append = [&](uint32_t i0, uint32_t i1, uint32_t i2, uint32_t submesh)
    {
        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size())
        {
            return;
        }
        const auto makePoint = [&](uint32_t index)
        {
            const auto value = uv(mesh.vertices[index]);
            return Point{
                std::clamp(value.x * (_resolution - 1), -4.0f * _resolution, 5.0f * _resolution),
                std::clamp(value.y * (_resolution - 1), -4.0f * _resolution, 5.0f * _resolution)};
        };
        if (!std::isfinite(uv(mesh.vertices[i0]).x) || !std::isfinite(uv(mesh.vertices[i0]).y) ||
            !std::isfinite(uv(mesh.vertices[i1]).x) || !std::isfinite(uv(mesh.vertices[i1]).y) ||
            !std::isfinite(uv(mesh.vertices[i2]).x) || !std::isfinite(uv(mesh.vertices[i2]).y))
        {
            return;
        }
        drawTriangle({makePoint(i0), makePoint(i1), makePoint(i2)}, kColors[submesh % kColors.size()]);
    };
    if (!mesh.submeshes.empty())
    {
        for (uint32_t s = 0; s < mesh.submeshes.size(); ++s)
        {
            const auto& submesh = mesh.submeshes[s];
            const uint64_t end = std::min<uint64_t>(static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount, mesh.indices.size());
            for (uint64_t i = submesh.firstIndex; i + 2 < end; i += 3)
            {
                append(mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2], s);
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            append(mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2], 0);
        }
    }
    const Color border{244, 246, 249, 255};
    line({0, 0}, {static_cast<float>(_resolution - 1), 0}, border);
    line({static_cast<float>(_resolution - 1), 0}, {static_cast<float>(_resolution - 1), static_cast<float>(_resolution - 1)}, border);
    line({static_cast<float>(_resolution - 1), static_cast<float>(_resolution - 1)}, {0, static_cast<float>(_resolution - 1)}, border);
    line({0, static_cast<float>(_resolution - 1)}, {0, 0}, border);
    _image->uploadRGBA8(_pixels.data(), {_resolution, _resolution});
}

}
