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

#include "uv_surface/probe.hpp"
#include <bg2e/bake/UvMapPreviewRenderer.hpp>
#include <array>

namespace bg2e::bake::tests {
inline std::shared_ptr<bg2e::geo::Mesh> retirementUvFixture()
{
    using namespace bg2e;
    auto mesh = std::make_shared<geo::Mesh>();

    const auto vertex = [](glm::vec3 position, glm::vec2 uv1, glm::vec2 uv2)
    {
        geo::VertexPNUUT result{};
        result.position = position;
        result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        result.texCoord0 = uv1;
        result.texCoord1 = uv2;
        result.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        return result;
    };

    mesh->vertices = {
        vertex({-0.8f, -0.8f, 0.0f}, {0.0f, 0.0f}, {0.1f, 0.1f}),
        vertex({-0.2f, -0.8f, 0.0f}, {1.0f, 0.0f}, {0.4f, 0.1f}),
        vertex({-0.8f, -0.2f, 0.0f}, {0.0f, 1.0f}, {0.1f, 0.4f}),
        vertex({0.2f, 0.2f, 0.0f}, {0.0f, 0.0f}, {0.6f, 0.6f}),
        vertex({0.8f, 0.2f, 0.0f}, {1.0f, 0.0f}, {0.9f, 0.6f}),
        vertex({0.8f, 0.8f, 0.0f}, {1.0f, 1.0f}, {0.9f, 0.9f})};
    mesh->indices = {0, 1, 2, 3, 4, 5};
    mesh->submeshes = {{0, 3}, {3, 3}};
    return mesh;
}

// Same two-island UV1/UV2 geometry as the retired registered fixture, using
// canonical top-row UV addressing and UINT_MAX uncovered identity.
inline void runRetirementUvProbe(draw::Engine& engine, const std::filesystem::path& shaders)
{
    auto mesh = retirementUvFixture();
    auto source = std::make_shared<draw::Drawable>();
    source->setMesh(mesh);
    source->load(&engine);
    UvSurfaceConfiguration configuration;
    configuration.shaderRoot = shaders;
    UvSurfacePass pass(configuration);
    pass.initialize(*engine.device(), engine.backendType(), 1);
    auto geometry = pass.prepare(source);
    pass.beginFrame(0, 1);
    UvSurfaceResult result;
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        result = pass.record({*command, 0, 1, 1, 1, geometry, glm::mat4(1), {64, 64}});
    });
    const auto diagnostic = UvSurfacePass::readDiagnostic(result);
    std::array<size_t, 2> counts{};
    size_t empty = 0;
    for (size_t i = 0; i < diagnostic.coverage.size(); ++i)
    {
        if (!diagnostic.coverage[i])
        {
            ++empty;
            if (diagnostic.submeshes[i] != UINT32_MAX || diagnostic.charts[i])
            {
                throw std::runtime_error("Uncovered UV texel lost its canonical typed identity clear");
            }
        }
        else
        {
            if (diagnostic.submeshes[i] >= counts.size() || !diagnostic.charts[i])
            {
                throw std::runtime_error("Covered UV texel lost submesh/chart identity");
            }
            ++counts[diagnostic.submeshes[i]];
        }
    }
    const auto index = [](float u, float v)
    {
        return size_t(v * 64) * 64 + size_t(u * 64);
    };
    if (!counts[0] || !counts[1] || !empty || diagnostic.submeshes[index(.2f, .2f)] != 0 ||
        diagnostic.submeshes[index(.75f, .7f)] != 1 || diagnostic.coverage[index(.5f, .5f)])
    {
        throw std::runtime_error("Two UV islands no longer retain separate coverage and submesh identity");
    }
    UvMapPreviewRenderer preview(*engine.device(), 64);
    const auto background = [&](float u, float v)
    {
        const auto i = index(u, v) * 4;
        const auto& p = preview.pixels();
        return p[i] == 18 && p[i + 1] == 21 && p[i + 2] == 27;
    };
    preview.render(*mesh, 0);
    if (background(.2f, .2f))
    {
        throw std::runtime_error("UV1 preview lost triangle coverage");
    }
    const auto uv1 = preview.pixels();
    preview.render(*mesh, 1);
    if (background(.2f, .2f) || !background(.5f, .5f) || uv1 == preview.pixels() || !preview.image()->isValid())
    {
        throw std::runtime_error("UV2 preview covered empty atlas space or failed to replace UV1");
    }
    bool boundary = false;
    preview.render(*mesh, 0);
    for (uint32_t i = 0; i < 64; ++i)
    {
        for (const auto offset : {size_t(i) * 4, (size_t(i) * 64) * 4})
        {
            const auto& p = preview.pixels();
            boundary |= p[offset] != 18 || p[offset + 1] != 21 || p[offset + 2] != 27;
        }
    }
    if (!boundary)
    {
        throw std::runtime_error("UV preview clipped every unit-square boundary edge");
    }
    pass.cleanup();
    source->cleanup();
}
}
