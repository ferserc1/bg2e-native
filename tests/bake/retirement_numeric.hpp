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

#include "integrated/probe.hpp"
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/gpu/Device.hpp>
#include <algorithm>
#include <cmath>

namespace bg2e::bake::tests {
inline std::shared_ptr<bg2e::geo::Mesh> makeTriangleMesh()
{
    using namespace bg2e;
    auto mesh = std::make_shared<geo::Mesh>();

    const auto vertex = [](glm::vec3 position, glm::vec2 uv)
    {
        geo::VertexPNUUT result{};
        result.position = position;
        result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        result.texCoord0 = uv;
        result.texCoord1 = uv;
        result.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        return result;
    };

    mesh->vertices = {
        vertex({-0.8f, -0.8f, 0.0f}, {0.1f, 0.1f}),
        vertex({0.8f, -0.8f, 0.0f}, {0.9f, 0.1f}),
        vertex({-0.8f, 0.8f, 0.0f}, {0.1f, 0.9f})};
    mesh->indices = {0, 1, 2};
    mesh->submeshes = {{0, 3}};
    return mesh;
}

inline std::shared_ptr<bg2e::geo::Mesh> makeOverhangMesh()
{
    using namespace bg2e;
    auto mesh = std::make_shared<geo::Mesh>();
    const auto vertex = [](glm::vec3 position, glm::vec2 uv)
    {
        geo::VertexPNUUT result{};
        result.position = position;
        result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        result.texCoord0 = uv;
        result.texCoord1 = uv;
        result.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        return result;
    };
    mesh->vertices = {
        vertex({-0.55f, -0.55f, 0.08f}, {0.0f, 0.0f}),
        vertex({-0.10f, -0.55f, 0.08f}, {1.0f, 0.0f}),
        vertex({-0.10f, -0.10f, 0.08f}, {1.0f, 1.0f}),
        vertex({-0.55f, -0.10f, 0.08f}, {0.0f, 1.0f})};
    mesh->indices = {0, 1, 2, 0, 2, 3};
    mesh->submeshes = {{0, 6}};
    return mesh;
}

inline void runRetirementNumericProbe(draw::Engine& engine,
    const std::filesystem::path& shaders,
    std::shared_ptr<const draw::environment::EnvironmentResources> environment)
{
    auto root = std::make_shared<scene::Node>("Numeric bake regression");
    auto target = std::make_shared<scene::Node>("Receiver");
    auto receiver = std::make_shared<draw::Drawable>();
    receiver->setMesh(makeTriangleMesh());
    receiver->load(&engine);
    target->addComponent(std::make_shared<scene::DrawableComponent>(receiver));
    root->addChild(target);
    BakerContextConfiguration configuration;
    configuration.device = engine.device();
    configuration.backend = engine.backendType();
    configuration.frameSlots = 2;
    configuration.shaderRoot = shaders;
    configuration.sceneRoot = root;
    configuration.environment = environment;
    IntegratedBakerContext context(engine, configuration);
    LightmapBakerConfiguration job;
    job.target = target;
    job.settings->setResolution(64);
    job.settings->setMode(LightmapMode::RTAO);
    job.settings->setCpuFormat(LightmapPixelFormat::RGB32F);
    job.settings->setAccumulationFrames(1);
    job.settings->setSamplesPerPixel(64);
    job.settings->setAoRadius(1);
    job.settings->setMaxRayDistance(1);
    auto baker = context.createBaker(job);
    uint64_t serial = 0;
    const auto record = [&]
    {
        ++serial;
        const auto slot = uint32_t(serial % 2);
        engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            BakeFrame frame{*command, slot, serial, context.sourceGeneration()};
            context.prepareFrame(frame);
            baker->record(frame);
        });
        context.completeFrame(slot, serial);
    };
    record();
    const auto isolated = baker->readPixels();
    const auto& neutral = std::get<std::vector<float>>(isolated.rgb);
    if (!std::all_of(neutral.begin(), neutral.end(), [](float v)
        {
            return v > .995f && v <= 1.001f;
        }))
    {
        throw std::runtime_error("Isolated receiver lost neutral AO over the complete atlas");
    }
    auto occluder = std::make_shared<draw::Drawable>();
    occluder->setMesh(makeOverhangMesh());
    occluder->material().setAlbedo(base::Color::Red());
    occluder->load(&engine);
    auto overhang = std::make_shared<scene::Node>("Overhang");
    overhang->addComponent(std::make_shared<scene::DrawableComponent>(occluder));
    root->addChild(overhang);
    context.invalidateSources();
    baker->resetAccumulation();
    if (baker->progress().completedSamples != 0)
    {
        throw std::runtime_error("Scene reset retained an old completed bake sample");
    }
    record();
    const auto ao = baker->readPixels();
    const auto& rgb = std::get<std::vector<float>>(ao.rgb);
    const auto at = [](float u, float v)
    {
        return (size_t(v * 64) * 64 + size_t(u * 64)) * 3;
    };
    if (rgb[at(.36f, .36f)] + .08f >= rgb[at(.15f, .15f)] || rgb[at(.75f, .75f)] < .995f)
    {
        throw std::runtime_error("Overhang did not selectively occlude UV texels or damaged neutral padding");
    }
    job.settings->setCpuFormat(LightmapPixelFormat::RGB8);
    baker->resetAccumulation();
    record();
    const auto bytes = baker->readPixels();
    if (std::get<std::vector<uint8_t>>(bytes.rgb).size() != 64 * 64 * 3 || baker->progress().completedSamples != 1)
    {
        throw std::runtime_error("Completed RGB8 result lost packing or exact reset sample count");
    }
    job.settings->setMode(LightmapMode::RTGI);
    job.settings->setCpuFormat(LightmapPixelFormat::RGB32F);
    job.settings->setGiBounces(2);
    job.settings->setSamplesPerPixel(64);
    baker->resetAccumulation();
    record();
    const auto gi = baker->readPixels();
    const auto& hdr = std::get<std::vector<float>>(gi.rgb);
    const auto i = at(.36f, .36f);
    if (hdr.size() != 64 * 64 * 3 ||
        !std::all_of(hdr.begin(), hdr.end(), [](float v)
        {
            return std::isfinite(v) && v >= 0;
        }) ||
        (std::abs(hdr[i] - hdr[i + 1]) <= .02f && std::abs(hdr[i + 1] - hdr[i + 2]) <= .02f))
    {
        throw std::runtime_error("RTGI lost finite, independently colored RGB32F output");
    }
    baker->cleanup();
    context.cleanup();
    occluder->cleanup();
    receiver->cleanup();
}
}
