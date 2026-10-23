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
#include <bg2e/draw/all.hpp>
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::tests {
// Stopped initialized host only. Source fixture: waits/readback are never production work.
// False reports unsupported v3 kernels/formats, never successful validation.
inline bool runTemporalReprojectionProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    auto& device = *engine.device();
    const auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    TemporalConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    configuration.metadataShaderInterface = "draw.temporal.v2";
    configuration.reprojectionShaderInterface = "draw.temporal.v3";
    TemporalAccumulator temporal(configuration);
    temporal.initialize(device,engine.backendType(),2);
    if (!temporal.surfaceReprojectionAvailable())
    {
        temporal.cleanup();
        return false;
    }
    temporal.setPositionThresholds(0.001f,0);
    auto upload = [&](gpu::Size2D size, gpu::PixelFormat format, const void* data, size_t bytes, size_t row)
    {
        gpu::ImageDescription description;
        description.size = size;
        description.format = format;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        auto image = device.createImage(description);
        image->upload({data,bytes,row,0,0});
        return image;
    };
    auto depth = [&](float value)
    {
        gpu::ImageDescription description;
        description.size = {3,3};
        description.format = gpu::PixelFormat::D32_SFLOAT;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::DepthStencil;
        auto image = device.createImage(description);
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(image.get(),gpu::ImageLayout::DepthAttachment);
            gpu::RenderingDescription rendering;
            rendering.depth = gpu::DepthAttachmentDescription{image.get(),gpu::AttachmentLoadOperation::Clear,
                gpu::AttachmentStoreOperation::Store,value};
            command->beginRendering(rendering);
            command->endRendering();
            command->transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
        });
        return image;
    };
    const auto oldDepth = depth(0.5f), currentDepth = depth(0.75f);
    std::array<uint32_t,9> tokens;
    tokens.fill(7);
    std::array<glm::vec4,9> expectedPositions;
    std::array<std::array<uint16_t,4>,9> expectedNormals, currentNormals;
    for (uint32_t y = 0; y < 3; ++y)
    {
        for (uint32_t x = 0; x < 3; ++x)
        {
            expectedPositions[y*3+x] = {2*(float(x)+0.5f)/3-1,1-2*(float(y)+0.5f)/3,0.5f,1};
        }
    }
    expectedNormals.fill({0,0,0x3c00,0x3c00}); // Signed previous +Z, valid.
    currentNormals.fill({0x3800,0x3800,0x3c00,0}); // Encoded current +Z.
    TemporalView view;
    // RH camera at world Z=1; preserve the synthetic identity view-projection.
    view.view[3].z = -1.0f;
    view.projection[3].z = 1.0f;
    view.jitteredProjection = view.projection;
    uint64_t sequence = 0;
    bool includePreviousSurface = true;
    auto render = [&](uint16_t value, const std::shared_ptr<gpu::Image>& receiver,
        const std::shared_ptr<gpu::Image>& metadataDepth)
    {
        const std::array<uint16_t,4> values{value,value,value,value};
        auto signal = upload({2,2},gpu::PixelFormat::R16_SFLOAT,values.data(),sizeof(values),4);
        auto token = upload({3,3},gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),12);
        auto normal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,currentNormals.data(),sizeof(currentNormals),24);
        TemporalSurfaceMetadata metadata{token,metadataDepth};
        if (includePreviousSurface)
        {
            metadata.previousPosition = upload({3,3},gpu::PixelFormat::R32G32B32A32_SFLOAT,
                expectedPositions.data(),sizeof(expectedPositions),48);
            metadata.previousNormal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,
                expectedNormals.data(),sizeof(expectedNormals),24);
            metadata.legacyCompatible = false;
        }
        SurfaceInputs surface{SurfaceProfile::CameraDepth,normal.get(),receiver.get(),nullptr,
            glm::inverse(view.jitteredProjection*view.view)};
        const uint32_t slot = uint32_t(sequence%2);
        temporal.beginFrame(slot,sequence+1,sequence,true);
        TemporalResult result;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            result = temporal.record({*command,slot,sequence+1,sequence,1,{},*signal,surface,view,nullptr,1,metadata});
        });
        ++sequence;
        return result;
    };
    const auto read = [&](const TemporalResult& result)
    {
        std::vector<uint8_t> bytes;
        result.image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        require(bytes.size() == 8,"V3 probe must preserve reduced 2x2 signal extent");
        std::array<uint16_t,4> output;
        std::memcpy(output.data(),bytes.data(),bytes.size());
        return output;
    };
    const auto first = render(0x3400,oldDepth,oldDepth); // .25
    view.view[3].x = 0.12f; // Exceeds legacy camera reset threshold.
    currentNormals.fill({0x3c00,0x3800,0x3800,0}); // Current +X; expected previous stays +Z.
    const auto moved = render(0x3a00,currentDepth,currentDepth); // .75 -> .5
    require(moved.frameCount == 2,"Supported camera movement must keep consecutive history");
    for (auto value : read(moved))
    {
        require(value == 0x3800,"Rigid motion must validate previous position/normal, not the changed current surface");
    }
    for (auto& position : expectedPositions)
    {
        position.x -= 0.12f;
        position.z = 0.75f;
    }
    expectedNormals.fill({0x3c00,0,0,0x3c00}); // Previous +X.
    tokens[0] = 9;
    const auto changedToken = read(render(0x3400,currentDepth,currentDepth));
    require(changedToken[0] == 0x3400 && changedToken[1] != 0x3400,
        "Changed token must restart only its matching receiver pixel");
    expectedPositions[0].w = 0;
    const auto invalidSurface = read(render(0x3a00,currentDepth,currentDepth));
    require(invalidSurface[0] == 0x3a00 && invalidSurface[1] != 0x3a00,
        "Invalid previous surface must restart locally while neighbors retain history");
    expectedPositions[0].w = 1;
    expectedPositions[0].x = 4;
    const auto outside = read(render(0x3400,currentDepth,currentDepth));
    require(outside[0] == 0x3400 && outside[1] != 0x3400,
        "Previous clip outside the screen must reject before integer pixel conversion");
    expectedNormals[2] = {0,0,0x3c00,0x3c00};
    const auto wrongNormal = read(render(0x3a00,currentDepth,currentDepth));
    require(wrongNormal[1] == 0x3a00 && wrongNormal[3] != 0x3a00,
        "Wrong expected previous normal must restart only the corresponding signal pixel");
    expectedNormals[2] = {0x3c00,0,0,0x3c00};
    for (auto value : read(render(0x3a00,currentDepth,oldDepth)))
    {
        require(value == 0x3a00,"Current metadata/receiver depth mismatch must reject even with valid tokens");
    }
    for (auto value : read(first))
    {
        require(value == 0x3400,"Retained pre-motion output must survive later slots and metadata owners");
    }
    includePreviousSurface = false;
    const auto legacy = render(0x3400,currentDepth,currentDepth);
    require(legacy.frameCount == 1,"Switching metadata profiles must restart local compatibility");
    includePreviousSurface = true;
    const auto restored = render(0x3a00,currentDepth,currentDepth);
    require(restored.frameCount == 1,"A history without previous-surface metadata cannot become rigid-valid");
    temporal.invalidate();
    require(render(0x3400,currentDepth,currentDepth).frameCount == 1,
        "Explicit camera cut must reset even supported reprojection");
    view.projection[0][0] = 1.1f;
    view.jitteredProjection = view.projection;
    require(render(0x3a00,currentDepth,currentDepth).frameCount == 1,
        "Projection changes must keep the conservative local reset");
    temporal.cleanup();
    return true;
}
}
