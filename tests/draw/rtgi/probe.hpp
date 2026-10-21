/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace bg2e::draw::tests {
// Stopped initialized Engine, outside frame acquisition. false means unavailable,
// not a passing trace. No renderer/camera is instantiated for explicit surfaces.
inline bool runRTGIProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    auto& device = *engine.device();
    RTGIConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    struct ObservedGI : RTGlobalIllumination {
        using RTGlobalIllumination::RTGlobalIllumination;
        uint32_t recorded = 0;

        void record(const RTGIContext& context) override
        {
            ++recorded;
            RTGlobalIllumination::record(context);
        }
    };
    ObservedGI gi(configuration);
    gi.initialize(device,engine.backendType(),1);
    if (!gi.available())
    {
        gi.cleanup();
        return false;
    }
    gi.setEnabled(true);
    gi.setQuality(RTGIQuality::Ultra);
    gi.setSampleCount(64);
    gi.setBounceCount(1);
    gi.setMaxDistance(10);
    gi.setBias(0.001f);
    environment::EnvironmentGenerator generator(engine,shaderRoot);
    auto environment = generator.build();
    auto root = std::make_shared<scene::Node>("GI fixture");
    auto plane = [&](float z, bool transparent)
    {
        auto mesh = std::make_shared<Drawable::Mesh>();
        for (auto xy : {glm::vec2(-10,-10),glm::vec2(10,-10),glm::vec2(10,10),glm::vec2(-10,10)})
        {
            mesh->vertices.push_back({{xy.x,xy.y,z},{0,0,-1},{0.25f,0.5f},{0.75f,0.5f},{1,0,0}});
        }
        mesh->indices = {0,1,2,0,1,2,0,2,3};
        mesh->submeshes = {{3,6}};
        auto drawable = std::make_shared<Drawable>();
        drawable->setMesh(mesh);
        drawable->material().setIsTransparent(transparent);
        drawable->setRayTracingEnabled(!transparent);
        drawable->load(&engine);
        auto node = std::make_shared<scene::Node>("GI plane");
        node->addComponent(new scene::DrawableComponent(drawable));
        root->addChild(node);
        return drawable;
    };
    auto ignored = plane(0.01f,true);
    auto nearPlane = plane(0.1f,false);
    auto farPlane = plane(0.5f,false);
    gpu::ImageDescription alphaDescription;
    alphaDescription.size = {2,1};
    alphaDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    auto alpha = device.createImage(alphaDescription);
    const uint8_t pixels[] = {255,255,255,0,255,255,255,255};
    alpha->upload({pixels,sizeof(pixels),8,0,0});
    auto texture = std::make_shared<Texture>(&engine);
    auto settings = std::make_shared<base::Texture>();
    settings->setMinFilter(base::Texture::FilterNearest);
    settings->setMagFilter(base::Texture::FilterNearest);
    texture->load(settings,alpha);
    nearPlane->drawMaterial(0)->setTexture(TextureSlot::Albedo,texture);
    nearPlane->material().setAlphaCutoff(0.5f);
    nearPlane->material().setAlbedoUVSet(1);
    RenderQueue queue;
    auto extract = [&]
    {
        RenderQueueVisitor visitor;
        visitor.enqueue(root.get(),engine,queue);
    };
    extract();
    auto first = gi.tracingResources()->prepare(queue, gi.tableCapacity());
    auto second = gi.tracingResources()->prepare(queue, gi.tableCapacity());
    if (first->scene == second->scene || first->instanceBuffer == second->instanceBuffer ||
        first->materialBuffer != second->materialBuffer || !first->shadowInstances ||
        first->shadowInstances != second->shadowInstances ||
        first->shadowTable.instances.size() != first->instances.size())
    {
        throw std::runtime_error("Unsubmitted snapshots must share material/shadow tables but retain separate pending acceleration");
    }
    for (size_t i = 0; i < first->instances.size(); ++i)
    {
        if (first->shadowTable.instances[i].ranges.w != first->instances[i].identity.x ||
            first->shadowTable.instances[i].ranges.x != first->instances[i].ranges.x ||
            first->shadowTable.instances[i].options.z != first->instances[i].ranges.y ||
            first->scene->instances()[i].instanceId != i)
        {
            throw std::runtime_error("Shared shadow/material tables must use identical dense TLAS order");
        }
    }
    if (first->geometry != second->geometry || first->instances.size() != 2 || first->instances[0].identity.x != 2 ||
        first->instances[1].identity.x != 3 || first->instances[0].ranges.x != 3 || first->instances[1].ranges.y != 4 ||
        first->geometry->vertices[0].uv != glm::vec4(0.25f,0.5f,0.75f,0.5f) ||
        first->materials[0].values.uvSets[0].x != 1 || first->owners[0].item.tracingMesh->description().opaque != true)
    {
        throw std::runtime_error("Shared tracing geometry, dense IDs, submesh bases, UV/material data or opacity differs");
    }
    auto sceneLayout = device.createPipelineLayout(pipeline::rtgiBindingLayout(
        gi.tableCapacity(), false, engine.backendType()));
    auto otherSceneLayout = device.createPipelineLayout(pipeline::rtgiBindingLayout(
        gi.tableCapacity(), false, engine.backendType()));
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        first->build(*command);
        const auto bindings = first->resourceSet(*sceneLayout);
        for (uint32_t composition = 0; composition < 23; ++composition)
        {
            if (first->resourceSet(*sceneLayout, 1, gi.tableCapacity()) != bindings)
            {
                throw std::runtime_error("Repeated compositions must reuse the snapshot's scene bindings");
            }
        }
        const auto other = first->resourceSet(*otherSceneLayout);
        if (other == bindings || first->resourceSet(*otherSceneLayout) != other)
        {
            throw std::runtime_error("Distinct pipeline layouts need independently cached scene bindings");
        }
        second->build(*command);
        if (first->materialResourceSet(*sceneLayout) != second->materialResourceSet(*sceneLayout))
        {
            throw std::runtime_error("Unchanged snapshots must reuse persistent image/sampler sets");
        }
        if (second->resourceSet(*sceneLayout) == bindings)
        {
            throw std::runtime_error("Scene bindings must not be cached across frame snapshots");
        }
    });
    auto upload = [&](gpu::PixelFormat format, const void* data, size_t size)
    {
        gpu::ImageDescription description;
        description.size = {1,1};
        description.format = format;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        auto image = device.createImage(description);
        image->upload({data,size,size,0,0});
        return image;
    };
    const uint16_t signedNormal[] = {0,0,0x3c00,0};
    const float position[] = {0,0,0,1};
    const uint8_t coverage = 255;
    auto normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,signedNormal,sizeof(signedNormal));
    auto world = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,position,sizeof(position));
    auto valid = upload(gpu::PixelFormat::R8_UNORM,&coverage,1);
    gpu::ImageDescription description;
    description.size = {1,1};
    description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    description.usage = gpu::ImageUsage::Storage | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc;
    if (!device.formatCapabilities(description.format).supports(description.usage))
    {
        gi.cleanup();
        return false;
    }
    auto output = device.createImage(description);
    SurfaceInputs surface{SurfaceProfile::WorldPositionCoverage,normal.get(),world.get(),valid.get()};
    auto invalid = surface;
    invalid.normal = world.get();
    bool rejected = false;
    try
    {
        invalid.validate(device);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    if (!rejected)
    {
        throw std::runtime_error("GI surface profile accepted an incompatible normal format");
    }
    uint64_t serial = 0;
    auto render = [&]
    {
        extract();
        gi.prepare(0,++serial,queue);
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            gi.record({*command,0,serial,0,surface,*output,queue.lights(),environment});
        });
        std::vector<uint8_t> bytes;
        output->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != 8)
        {
            throw std::runtime_error("GI HDR readback size differs");
        }
        glm::vec4 value(0);
        for (uint32_t c = 0; c < 4; ++c)
        {
            uint16_t half;
            std::memcpy(&half,bytes.data()+c*2,2);
            const uint32_t exponent = (half >> 10) & 31u, mantissa = half & 1023u;
            value[c] = exponent ? std::ldexp(float(1024u+mantissa),int(exponent)-25) : std::ldexp(float(mantissa),-24);
            if ((half & 0x8000u) || exponent == 31u || !std::isfinite(value[c]))
            {
                throw std::runtime_error("GI HDR output is negative or nonfinite");
            }
        }
        return value;
    };
    nearPlane->material().setAlbedo(base::Color(1,0,0,1));
    farPlane->material().setAlbedo(base::Color(0,0,1,1));
    nearPlane->material().setLightEmission(0);
    farPlane->material().setLightEmission(0);
    const auto baseline = render();
    nearPlane->material().setLightEmission(2);
    const auto nearGI = render();
    if (nearGI.a < 0.99f || nearGI.r < baseline.r+1.0f || nearGI.r <= nearGI.b)
    {
        throw std::runtime_error("GI emissive hit, albedo or HDR validity differs");
    }
    farPlane->material().setLightEmission(2);
    nearPlane->material().setAlbedoUVSet(0);
    const auto farGI = render();
    if (farGI.a < 0.99f || farGI.b < 1.0f || farGI.b <= farGI.r)
    {
        throw std::runtime_error("GI UV0 cutout did not continue to the blue rear emitter");
    }
    gi.setMaxDistance(0.2f);
    const auto escaped = render();
    farPlane->setRayTracingEnabled(false);
    gi.setMaxDistance(10);
    const auto excluded = render();
    if (escaped.a < 0.99f || glm::length(escaped-excluded) > 0.01f)
    {
        throw std::runtime_error("GI distance bound and excluded rear geometry disagree on environment escape");
    }
    farPlane->setRayTracingEnabled(true);
    gi.setBounceCount(2);
    auto noise = std::make_shared<utils::BlueNoise>(device);
    noise->build();
    gi.setNoise(noise);
    if (render().a < 0.99f)
    {
        throw std::runtime_error("GI blue-noise/bounce transport is invalid");
    }
    // A point below the emitters illuminates their downward-facing hit normals.
    // Isolate direct light transport from emission and verify participation.
    gi.setUseBlueNoise(false);
    gi.setBounceCount(1);
    farPlane->material().setLightEmission(0);
    nearPlane->material().setLightEmission(0);
    nearPlane->material().setAlbedoUVSet(1);
    LightUniforms injectedLights;
    injectedLights.count.x = 1;
    injectedLights.lights[0].positionIntensity = {0,0,-1,20};
    injectedLights.lights[0].color = {1,1,1,1};
    injectedLights.lights[0].flags = {0,1,1,1};
    auto renderWithLights = [&]
    {
        extract();
        gi.prepare(0,++serial,queue);
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            gi.record({*command,0,serial,0,surface,*output,injectedLights,environment});
        });
        std::vector<uint8_t> bytes;
        output->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        uint16_t half;
        std::memcpy(&half,bytes.data(),2);
        const uint32_t exponent = (half >> 10) & 31u, mantissa = half & 1023u;
        return exponent ? std::ldexp(float(1024u+mantissa),int(exponent)-25) : std::ldexp(float(mantissa),-24);
    };
    const float direct = renderWithLights();
    injectedLights.lights[0].flags.w = 0;
    if (direct <= renderWithLights()+0.1f)
    {
        throw std::runtime_error("GI direct light participation did not change hit illumination");
    }
    // Camera and explicit surfaces reconstruct the same world receiver.
    const auto explicitGI = render();
    const uint16_t encodedNormal[] = {0x3800,0x3800,0x3c00,0};
    auto encoded = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,encodedNormal,sizeof(encodedNormal));
    gpu::ImageDescription depthDescription;
    depthDescription.size = {1,1};
    depthDescription.format = gpu::PixelFormat::D32_SFLOAT;
    depthDescription.usage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled;
    auto depth = device.createImage(depthDescription);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        command->transition(depth.get(),gpu::ImageLayout::DepthAttachment);
        gpu::RenderingDescription rendering;
        rendering.depth = gpu::DepthAttachmentDescription{depth.get(),gpu::AttachmentLoadOperation::Clear,
            gpu::AttachmentStoreOperation::Store,0};
        command->beginRendering(rendering);
        command->endRendering();
        command->transition(depth.get(),gpu::ImageLayout::ShaderReadOnly);
    });
    surface = {SurfaceProfile::CameraDepth,encoded.get(),depth.get(),nullptr,glm::mat4(1)};
    if (glm::length(render()-explicitGI) > 0.01f)
    {
        throw std::runtime_error("GI camera and explicit receiver profiles disagree");
    }
    gi.setEnabled(false);
    if (render() != glm::vec4(0) || gi.recorded < 5)
    {
        throw std::runtime_error("Disabled GI must return invalid zero HDR, preserving IBL fallback");
    }
    gi.setEnabled(true);
    root = std::make_shared<scene::Node>("empty GI scene");
    if (render() != glm::vec4(0))
    {
        throw std::runtime_error("Empty GI must return invalid zero HDR");
    }
    gi.cleanup();
    first.reset();
    second.reset();
    environment.reset();
    generator.cleanup();
    return true;
}
}
