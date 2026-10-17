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

#include <bg2e/draw/pipeline/RTReflections.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace bg2e::draw::tests {
// Stopped initialized Engine, outside frame acquisition. false means unavailable,
// not a passing trace. No renderer/camera is instantiated for explicit surfaces.
inline bool runRTReflectionsProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    auto& device = *engine.device();
    RTReflectionConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    struct ObservedReflection : RTReflections {
        using RTReflections::RTReflections;
        uint32_t recorded = 0;

        void record(const RTReflectionContext& context) override
        {
            ++recorded;
            RTReflections::record(context);
        }
    };
    ObservedReflection reflections(configuration);
    reflections.initialize(device,engine.backendType(),1);
    if (!reflections.available())
    {
        reflections.cleanup();
        return false;
    }
    reflections.setEnabled(true);
    reflections.setQuality(RTReflectionQuality::Ultra);
    reflections.setSampleCount(64);
    reflections.setMaxDistance(10);
    reflections.setBias(0.001f);
    environment::EnvironmentGenerator generator(engine,shaderRoot);
    auto environment = generator.build();
    auto root = std::make_shared<scene::Node>("Reflection fixture");
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
        auto node = std::make_shared<scene::Node>("Reflection plane");
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
    auto first = reflections.tracingResources()->prepare(queue);
    auto second = reflections.tracingResources()->prepare(queue);
    if (first->geometry != second->geometry || first->instances.size() != 2 || first->instances[0].identity.x != 2 ||
        first->instances[1].identity.x != 3 || first->instances[0].ranges.x != 3 || first->instances[1].ranges.y != 4 ||
        first->geometry->vertices[0].uv != glm::vec4(0.25f,0.5f,0.75f,0.5f) ||
        first->materials[0].values.uvSets[0].x != 1 || first->owners[0].item.tracingMesh->description().opaque != true)
    {
        throw std::runtime_error("Shared tracing geometry, dense IDs, submesh bases, UV/material data or opacity differs");
    }
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
        reflections.cleanup();
        return false;
    }
    auto output = device.createImage(description);
    gpu::Size2D displayExtent{1,1};
    const uint8_t mirrorMaterial[] = {0,0,255,0};
    auto material = upload(gpu::PixelFormat::R8G8B8A8_UNORM,mirrorMaterial,4);
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
        throw std::runtime_error("Reflection surface profile accepted an incompatible normal format");
    }
    uint64_t serial = 0;
    auto render = [&]
    {
        extract();
        reflections.prepare(0,++serial,queue);
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            reflections.record({*command,0,serial,0,surface,*output,*material,displayExtent,{0,0,1},queue.lights(),environment});
        });
        std::vector<uint8_t> bytes;
        output->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != size_t(output->width())*output->height()*8)
        {
            throw std::runtime_error("Reflection HDR readback size differs");
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
                throw std::runtime_error("Reflection HDR output is negative or nonfinite");
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
    const auto nearReflection = render();
    if (nearReflection.a < 0.999f || nearReflection.r < baseline.r+1.5f || nearReflection.r <= nearReflection.b)
    {
        throw std::runtime_error("Mirror HDR emitter radiance/confidence differs");
    }
    farPlane->material().setLightEmission(2);
    nearPlane->material().setAlbedoUVSet(0);
    const auto farReflection = render();
    if (farReflection.a < 0.999f || farReflection.b < 1.5f || farReflection.b <= farReflection.r)
    {
        throw std::runtime_error("Mirror cutout must reveal the blue rear emitter");
    }
    reflections.setMaxDistance(0.2f);
    if (render() != glm::vec4(0))
    {
        throw std::runtime_error("Reflection distance miss must return zero confidence for IBL fallback");
    }
    reflections.setMaxDistance(10);
    nearPlane->setRayTracingEnabled(false);
    farPlane->setSubmeshTransform(glm::scale(glm::mat4(1),glm::vec3(0.01f,0.01f,1)));
    const uint8_t glossyMaterial[] = {0,128,255,0};
    material = upload(gpu::PixelFormat::R8G8B8A8_UNORM,glossyMaterial,4);
    const auto partial = render();
    if (partial.a <= 0 || partial.a >= 1 || partial.b < 1.5f)
    {
        throw std::runtime_error("Glossy confidence must count partial geometry hits without premultiplying HDR radiance");
    }
    reflections.setMaxRoughness(0.25f);
    if (render() != glm::vec4(0))
    {
        throw std::runtime_error("Rejected roughness must preserve IBL through zero confidence");
    }
    reflections.setMaxRoughness(0.75f);
    material = upload(gpu::PixelFormat::R8G8B8A8_UNORM,mirrorMaterial,4);
    farPlane->setSubmeshTransform(glm::mat4(1));
    nearPlane->setRayTracingEnabled(true);
    nearPlane->material().setAlbedoUVSet(1);
    auto noise = std::make_shared<utils::BlueNoise>(device);
    noise->build();
    reflections.setNoise(noise);
    if (glm::length(render()-nearReflection) > 0.01f)
    {
        throw std::runtime_error("Mirror result must remain deterministic with shared blue noise");
    }
    reflections.setUseBlueNoise(false);
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
    if (glm::length(render()-nearReflection) > 0.01f)
    {
        throw std::runtime_error("Camera and explicit mirror receivers disagree");
    }
    // Keep the receiver at 1x1 while changing display-relative reflection quality.
    displayExtent = {3,3};
    reflections.setQuality(RTReflectionQuality::High);
    description.size = reflections.outputExtent(displayExtent);
    if (description.size != gpu::Size2D{2,2})
    {
        throw std::runtime_error("Reflection quality must be relative to display extent");
    }
    output = device.createImage(description);
    if (glm::length(render()-nearReflection) > 0.01f)
    {
        throw std::runtime_error("Display-relative reflection target must sample an independent receiver extent");
    }
    reflections.setEnabled(false);
    if (render() != glm::vec4(0) || reflections.recorded < 8)
    {
        throw std::runtime_error("Disabled reflection must preserve IBL and virtual dispatch");
    }
    reflections.setEnabled(true);
    root = std::make_shared<scene::Node>("empty reflection scene");
    if (render() != glm::vec4(0))
    {
        throw std::runtime_error("Empty reflection scene must preserve IBL");
    }
    reflections.cleanup();
    first.reset();
    second.reset();
    environment.reset();
    generator.cleanup();
    return true;
}
}
