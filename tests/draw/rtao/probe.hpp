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

#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace bg2e::draw::tests {
// Stopped initialized Engine, outside frame acquisition. false means unavailable,
// not a passing trace. No renderer/camera is instantiated for explicit surfaces.
inline bool runRTAOProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    auto& device = *engine.device();
    RTAOConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    struct ObservedAO : RTAmbientOcclusion {
        using RTAmbientOcclusion::RTAmbientOcclusion;
        uint32_t recorded = 0;

        void record(const RTAOContext& context) override
        {
            ++recorded;
            RTAmbientOcclusion::record(context);
        }
    };
    ObservedAO ao(configuration);
    ao.initialize(device,engine.backendType(),1);
    if (!ao.available())
    {
        ao.cleanup();
        return false;
    }
    ao.setEnabled(true);
    ao.setQuality(RTAOQuality::Ultra);
    ao.setSampleCount(64);
    ao.setBounceCount(1);
    ao.setRadius(1);
    ao.setBias(0.001f);
    auto root = std::make_shared<scene::Node>("AO fixture");
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
        auto node = std::make_shared<scene::Node>("AO plane");
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
    auto first = ao.tracingResources()->prepare(queue);
    auto second = ao.tracingResources()->prepare(queue);
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
    description.format = gpu::PixelFormat::R16_SFLOAT;
    description.usage = gpu::ImageUsage::Storage | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc;
    if (!device.formatCapabilities(description.format).supports(description.usage))
    {
        ao.cleanup();
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
        throw std::runtime_error("AO surface profile accepted an incompatible normal format");
    }
    uint64_t serial = 0;
    auto render = [&]
    {
        extract();
        ao.prepare(0,++serial,queue);
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            ao.record({*command,0,serial,0,surface,*output});
        });
        std::vector<uint8_t> bytes;
        output->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != sizeof(uint16_t))
        {
            throw std::runtime_error("AO scalar readback size differs");
        }
        uint16_t half;
        std::memcpy(&half,bytes.data(),sizeof(half));
        const uint32_t exponent = (half >> 10) & 31u, mantissa = half & 1023u;
        const float value = exponent ? std::ldexp(float(1024u+mantissa),int(exponent)-25) : std::ldexp(float(mantissa),-24);
        if ((half & 0x8000u) || exponent == 31u || value < 0 || value > 1)
        {
            throw std::runtime_error("AO scalar signal is outside [0,1] or nonfinite");
        }
        return value;
    };
    const float nearAO = render();
    nearPlane->material().setAlbedoUVSet(0); // Reject near candidates and keep traversing to the farther plane.
    const float farAO = render();
    if (nearAO >= 0.95f || farAO <= nearAO+0.05f)
    {
        throw std::runtime_error("AO cutout continuation/UV1/radius falloff did not reveal the farther hit");
    }
    ao.setRadius(0.2f);
    if (render() < 0.999f)
    {
        throw std::runtime_error("AO radius must exclude the far plane through near cutouts");
    }
    ao.setRadius(1);
    farPlane->setRayTracingEnabled(false);
    if (render() < 0.999f)
    {
        throw std::runtime_error("Rejected cutouts and excluded geometry must yield neutral AO");
    }
    auto noise = std::make_shared<utils::BlueNoise>(device);
    noise->build();
    ao.setNoise(noise);
    if (render() < 0.999f)
    {
        throw std::runtime_error("Blue-noise variant changed neutral AO");
    }
    ao.setEnabled(false);
    if (render() < 0.999f || ao.recorded < 6)
    {
        throw std::runtime_error("Disabled AO or virtual pass dispatch differs");
    }
    // Camera profile uses identity inverse VP and a cleared depth=0 at the same
    // center world position. Upload encoded normal; depth is cleared as attachment.
    ao.setEnabled(true);
    farPlane->setRayTracingEnabled(true);
    nearPlane->material().setAlbedoUVSet(1);
    ao.setUseBlueNoise(false);
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
    if (std::abs(render()-nearAO) > 0.005f)
    {
        throw std::runtime_error("Camera-depth and explicit world surfaces disagree at the same receiver");
    }
    root = std::make_shared<scene::Node>("empty AO scene");
    if (render() < 0.999f)
    {
        throw std::runtime_error("An empty tracing snapshot must yield neutral AO");
    }
    ao.cleanup();
    return true;
}
}
