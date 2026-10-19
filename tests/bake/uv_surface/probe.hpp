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
#include <bg2e/bake/all.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace bg2e::bake::tests {
inline geo::Mesh uvSurfaceQuad()
{
    geo::Mesh mesh;
    for (auto uv : {glm::vec2(0.1f,0.15f),glm::vec2(0.85f,0.15f),
        glm::vec2(0.85f,0.7f),glm::vec2(0.1f,0.7f)})
    {
        mesh.vertices.push_back({glm::vec3(uv,0),glm::vec3(0,0,1),uv,uv,glm::vec3(1,0,0)});
    }
    mesh.indices = {0,1,2,0,2,3};
    mesh.submeshes = {{0,6}};
    return mesh;
}

inline void runUvSurfaceCpuProbe()
{
    auto mesh = uvSurfaceQuad();
    auto packed = buildUvSurfaceMesh(mesh);
    if (packed.chartCount != 1 || packed.vertices.size() != 6 || packed.ranges[0].indexCount != 6)
    {
        throw std::runtime_error("Shared UV edge must join one chart with deindexed triangles");
    }
    auto seam = mesh;
    seam.vertices.push_back(mesh.vertices[0]);
    seam.vertices.push_back(mesh.vertices[2]);
    seam.indices = {0,1,2,4,5,3};
    if (buildUvSurfaceMesh(seam).chartCount != 2)
    {
        throw std::runtime_error("Coincident edge coordinates must not weld duplicated UV seam vertices");
    }
    mesh.submeshes = {{0,3},{3,3}};
    packed = buildUvSurfaceMesh(mesh);
    if (packed.chartCount != 2 || packed.vertices[0].identity.x != 0 || packed.vertices[3].identity.x != 1 ||
        packed.vertices[0].identity.y == packed.vertices[3].identity.y)
    {
        throw std::runtime_error("Submesh/material boundaries must remain isolated charts");
    }
    auto reject = [](const auto& operation)
    {
        bool failed = false;
        try
        {
            operation();
        }
        catch (const std::exception&)
        {
            failed = true;
        }
        if (!failed)
        {
            throw std::runtime_error("Invalid UV surface unexpectedly accepted");
        }
    };
    auto invalid = mesh;
    invalid.indices[0] = 999;
    reject([&]
    {
        buildUvSurfaceMesh(invalid);
    });
    invalid = mesh;
    invalid.vertices[0].texCoord1.x = std::numeric_limits<float>::quiet_NaN();
    reject([&]
    {
        buildUvSurfaceMesh(invalid);
    });
    invalid = mesh;
    invalid.indices = {0,1,2,0,1,2};
    if (buildUvSurfaceMesh(invalid).vertices.size() != 6)
    {
        throw std::runtime_error("Overlapping UV triangles must remain packable for baking");
    }
    invalid = mesh;
    invalid.vertices[0].normal = glm::vec3(0);
    reject([&]
    {
        buildUvSurfaceMesh(invalid);
    });
}

// Stopped initialized Engine and deployed native bake shaders. Throws on unsupported
// required formats/kernels (not a successful pass). No RT is needed by UV recording.
inline void runUvSurfaceGpuProbe(draw::Engine& engine, const std::filesystem::path& shaderRoot)
{
    auto source = std::make_shared<draw::Drawable>();
    source->setMesh(std::make_shared<geo::Mesh>(uvSurfaceQuad()));
    source->load(&engine);
    auto oldCpu = source->mesh();
    auto oldGpu = source->drawMesh();
    UvSurfaceConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    UvSurfacePass pass(configuration);
    pass.initialize(*engine.device(),engine.backendType(),2);
    auto geometry = pass.prepare(source);
    uint64_t serial = 1;
    const gpu::Size2D extent{33,17};
    glm::mat4 model = glm::translate(glm::mat4(1),glm::vec3(2,3,4));
    model = glm::rotate(model,0.4f,glm::vec3(1,0,0));
    model = glm::scale(model,glm::vec3(-2,3,0.5f));
    pass.beginFrame(0,serial);
    UvSurfaceResult first, shifted;
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        first = pass.record({*command,0,serial,7,9,geometry,model,extent});
        shifted = pass.record({*command,0,serial,7,10,geometry,
            glm::translate(glm::mat4(1),glm::vec3(10,0,0)),extent});
    });
    auto pixels = UvSurfacePass::readDiagnostic(first);
    const auto expectedNormal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model)))*glm::vec3(0,0,1));
    uint32_t covered = 0;
    for (uint32_t y = 0; y < extent.height; ++y)
    {
        for (uint32_t x = 0; x < extent.width; ++x)
        {
            const auto index = size_t(y)*extent.width+x;
            const float u = (x+0.5f)/extent.width, v = (y+0.5f)/extent.height;
            const bool inside = u > 0.1f && u < 0.85f && v > 0.15f && v < 0.7f;
            if ((pixels.coverage[index] != 0) != inside)
            {
                throw std::runtime_error("UV coverage/orientation/odd-extent border mismatch");
            }
            if (inside)
            {
                ++covered;
                const auto expectedPosition = model*glm::vec4(u,v,0,1);
                if (pixels.submeshes[index] != 0 || pixels.charts[index] != 1 ||
                    glm::length(pixels.positions[index]-expectedPosition) > 0.002f ||
                    glm::length(glm::vec3(pixels.normals[index])-expectedNormal) > 0.002f)
                {
                    throw std::runtime_error("UV integer identity/world transform/signed normal mismatch");
                }
            }
            else if (pixels.charts[index] || pixels.submeshes[index] != std::numeric_limits<uint32_t>::max())
            {
                throw std::runtime_error("UV integer attachment typed clear mismatch");
            }
        }
    }
    if (!covered || first.images[0] == shifted.images[0] || first.sourceGeneration != 7 || first.targetGeneration != 9)
    {
        throw std::runtime_error("UV invocation outputs/generations must remain independent");
    }
    auto shiftedPixels = UvSurfacePass::readDiagnostic(shifted);
    if (glm::length(shiftedPixels.positions[8*extent.width+16]-glm::vec4(10.5f,0.5f,0,1)) > 0.002f)
    {
        throw std::runtime_error("Second UV target overwrote/borrowed another invocation's transform");
    }
    ++serial;
    pass.beginFrame(0,serial); // Previous slot completed by immediateSubmit.
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        pass.record({*command,0,serial,7,11,geometry,glm::mat4(1),{7,5}});
    });
    const auto retained = UvSurfacePass::readDiagnostic(first);
    if (retained.coverage != pixels.coverage || retained.charts != pixels.charts ||
        glm::length(retained.positions[8*extent.width+16]-pixels.positions[8*extent.width+16]) > 0.0001f)
    {
        throw std::runtime_error("Completed-slot reuse overwrote externally retained UV output");
    }
    auto reloaded = generateUv2AndReload(source,{32,2});
    if (!reloaded.drawable->isLoaded() || source->mesh() != oldCpu || source->drawMesh() != oldGpu ||
        reloaded.drawable->mesh() == oldCpu || reloaded.drawable->drawMesh() == oldGpu ||
        reloaded.drawable->drawMaterial(0) == source->drawMaterial(0) ||
        !geo::UvAtlasValidator::validate(*reloaded.drawable->mesh(),1).valid ||
        reloaded.drawable->mesh()->indices.size() != source->mesh()->indices.size())
    {
        throw std::runtime_error("UV2 reload changed source/clone identity or lost atlas/material ownership");
    }
    if (source->rayTracingEnabled() && reloaded.drawable->rayTracingMesh(0) == source->rayTracingMesh(0))
    {
        throw std::runtime_error("UV2 reload must build fresh tracing geometry on available hardware");
    }
    if (glm::length(source->mesh()->vertices[0].texCoord1-glm::vec2(0.1f,0.15f)) != 0)
    {
        throw std::runtime_error("UV2 generator mutated another attachment's original CPU mesh");
    }
    for (const auto& vertex : reloaded.drawable->mesh()->vertices)
    {
        if (glm::length(vertex.texCoord0-glm::vec2(vertex.position)) > 1e-6f ||
            glm::length(vertex.normal-glm::vec3(0,0,1)) > 1e-6f ||
            glm::length(vertex.tangent-glm::vec3(1,0,0)) > 1e-6f)
        {
            throw std::runtime_error("UV2 generation changed UV1/position/normal/tangent payload");
        }
    }
    auto generatedGeometry = pass.prepare(reloaded.drawable);
    ++serial;
    pass.beginFrame(1,serial);
    UvSurfaceResult generatedSurface;
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        generatedSurface = pass.record({*command,1,serial,8,12,generatedGeometry,glm::mat4(1),{32,32}});
    });
    const auto generatedPixels = UvSurfacePass::readDiagnostic(generatedSurface);
    if (std::none_of(generatedPixels.coverage.begin(),generatedPixels.coverage.end(),[](uint8_t value)
        {
            return value != 0;
        }))
    {
        throw std::runtime_error("Generated UV2 reload produced no rasterized surface");
    }
    pass.cleanup();
    reloaded.drawable->cleanup();
    source->cleanup();
}
}
