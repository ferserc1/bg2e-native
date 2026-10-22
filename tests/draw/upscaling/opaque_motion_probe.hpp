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
#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::upscaling_probe {
// SDK-independent native selector fixture. Stopped initialized host only.
// False is unsupported format/shader, never a successful native acceptance result.
inline bool runOpaqueMotionProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    auto& device = *engine.device();
    const auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    const gpu::Size2D extent{5,3};
    const auto sampledUpload = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    const auto outputUsage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    if (!device.formatCapabilities(gpu::PixelFormat::R32_UINT).supports(sampledUpload) ||
        !device.formatCapabilities(gpu::PixelFormat::R16G16_SFLOAT).supports(sampledUpload | outputUsage) ||
        !device.formatCapabilities(gpu::PixelFormat::D32_SFLOAT).supports(gpu::ImageUsage::Sampled | gpu::ImageUsage::DepthStencil))
    {
        return false;
    }
    gpu::PipelineLayoutDescription cameraLayout;
    cameraLayout.resourceBindings = {
        {0,{0,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment,1},
        {0,{1,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment,1}};
    cameraLayout.pushConstants.push_back({0,128,gpu::ShaderStage::Fragment});
    gpu::PipelineLayoutDescription opaqueLayout;
    for (uint32_t i = 0; i < 3; ++i)
    {
        opaqueLayout.resourceBindings.push_back({0,{i,i},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment,1});
        opaqueLayout.resourceBindings.push_back({0,{i+3,i},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment,1});
    }
    opaqueLayout.pushConstants.push_back({0,128,gpu::ShaderStage::Fragment});
    pipeline::FullscreenPass camera, opaque;
    try
    {
        camera.initialize(engine,shaderRoot,"draw/upscale/motion",device.createPipelineLayout(cameraLayout),gpu::PixelFormat::R16G16_SFLOAT,2);
        opaque.initialize(engine,shaderRoot,"draw/upscale/motion_opaque",device.createPipelineLayout(opaqueLayout),gpu::PixelFormat::R16G16_SFLOAT,2);
    }
    catch (const std::exception&)
    {
        opaque.cleanup();
        camera.cleanup();
        return false;
    }
    auto upload = [&](gpu::Size2D size, gpu::PixelFormat format, const void* data, size_t bytes, size_t row)
    {
        gpu::ImageDescription description;
        description.size = size;
        description.format = format;
        description.usage = sampledUpload;
        auto image = device.createImage(description);
        image->upload({data,bytes,row,0,0});
        return image;
    };
    std::array<uint32_t,15> tokens{};
    tokens[0] = 7;
    tokens[1] = 8; // Valid stationary zero vector must override nonzero camera fallback.
    tokens[2] = tokens[4] = 9;
    tokens[5] = 0xffffffffu; // Integer identity must never pass through half/float.
    std::array<std::array<uint16_t,2>,15> vectors{};
    vectors[0] = vectors[3] = {0xc200,0x4400}; // (-3,4); token[3] is invalid.
    vectors[2] = {0x7e00,0x3c00}; // NaN -> camera fallback despite valid token.
    vectors[4] = {0x7c00,0x3c00}; // Inf -> camera fallback.
    vectors[5] = {0x3800,0xb800}; // (.5,-.5), preserved pixel units/sign.
    auto token = upload(extent,gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),20);
    auto motion = upload(extent,gpu::PixelFormat::R16G16_SFLOAT,vectors.data(),sizeof(vectors),20);
    UpscalerOpaqueMotion owned{motion.get(),token.get(),motion,token};
    owned.validate(device,extent);
    UpscalerOpaqueMotion{motion.get(),token.get()}.validate(device,extent); // Explicit borrowed lifetime.
    UpscalerOpaqueMotion{}.validate(device,extent); // Legacy caller.
    const std::array<uint16_t,2> oneVector{};
    auto small = upload({1,1},gpu::PixelFormat::R16G16_SFLOAT,oneVector.data(),sizeof(oneVector),4);
    gpu::ImageDescription noSampleDescription;
    noSampleDescription.size = extent;
    noSampleDescription.format = gpu::PixelFormat::R16G16_SFLOAT;
    noSampleDescription.usage = gpu::ImageUsage::TransferDst;
    auto noSample = device.createImage(noSampleDescription);
    const std::array<UpscalerOpaqueMotion,8> invalid{{
        {motion.get(),nullptr},{nullptr,token.get()},
        {motion.get(),token.get(),token,{}},{motion.get(),token.get(),{},motion},
        {token.get(),token.get()},{motion.get(),motion.get()},
        {small.get(),token.get()},{noSample.get(),token.get()}}};
    for (const auto& input : invalid)
    {
        bool rejected = false;
        try
        {
            input.validate(device,extent);
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        require(rejected,"Malformed opaque motion must reject rather than silently binding mismatched inputs");
    }
    gpu::ImageDescription description;
    description.size = extent;
    description.format = gpu::PixelFormat::D32_SFLOAT;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::DepthStencil;
    auto depth = device.createImage(description);
    description.format = gpu::PixelFormat::R16G16_SFLOAT;
    description.usage = outputUsage;
    auto cameraOutput = device.createImage(description);
    auto mixedOutput = device.createImage(description);
    struct Push {
        glm::mat4 current{1}, previous{1};
    } push;
    static_assert(sizeof(Push) == 128);
    push.previous[3].x = 2.0f/float(extent.width);
    push.previous[3].y = -4.0f/float(extent.height); // Camera fallback is (+1,+2) internal pixels.
    uint64_t serial = 0;
    for (const float receiverDepth : {0.5f,1.0f}) // Include uncovered/sky depth.
    {
        ++serial;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(depth.get(),gpu::ImageLayout::DepthAttachment);
            gpu::RenderingDescription rendering;
            rendering.depth = gpu::DepthAttachmentDescription{depth.get(),gpu::AttachmentLoadOperation::Clear,
                gpu::AttachmentStoreOperation::Store,receiverDepth};
            command->beginRendering(rendering);
            command->endRendering();
            FrameContext frame{engine,*command,*mixedOutput,extent,serial,uint32_t((serial-1)%2),0,serial-1,0};
            camera.draw(frame,*cameraOutput,{depth.get()},0,{},&push,sizeof(push));
            opaque.draw(frame,*mixedOutput,{depth.get(),owned.token,owned.motion},0,{},&push,sizeof(push));
        });
        std::vector<uint8_t> cameraBytes, mixedBytes;
        cameraOutput->readPixels(cameraBytes,{},gpu::ImageLayout::ShaderReadOnly);
        mixedOutput->readPixels(mixedBytes,{},gpu::ImageLayout::ShaderReadOnly);
        require(cameraBytes.size() == 60 && mixedBytes.size() == 60,"Opaque selection must preserve odd internal extent and RG16F output");
        std::array<std::array<uint16_t,2>,15> reference, actual;
        std::memcpy(reference.data(),cameraBytes.data(),60);
        std::memcpy(actual.data(),mixedBytes.data(),60);
        for (size_t i = 0; i < actual.size(); ++i)
        {
            const auto expected = (i == 0 || i == 1 || i == 5) ? vectors[i] : reference[i];
            require(actual[i] == expected,"Opaque selector must preserve valid object vectors and camera fallback elsewhere");
            const std::array<uint16_t,2> cameraPixels{0x3c00,0x4000};
            require(reference[i] == cameraPixels,"Camera fallback must retain pixel units, previous-current sign and framebuffer Y");
        }
    }
    opaque.cleanup();
    camera.cleanup();
    return true;
}
}
