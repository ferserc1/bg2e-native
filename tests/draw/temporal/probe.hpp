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
#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <cmath>
#include <cstring>

namespace bg2e::draw::tests {
inline void runMotionVectorReferenceProbe()
{
    using namespace pipeline;
    TemporalView current, previous;
    current.jitteredProjection[3].x = 0.2f;
    auto transforms = MotionVectorGenerator::camera(current,previous);
    const glm::vec4 clip(0.2f,0,0,1);
    if (glm::length(MotionVectorGenerator::motionPixels(transforms,clip,{100,100})) > 1e-5f)
    {
        throw std::runtime_error("Jitter alone must not create unjittered camera motion");
    }
    previous.view[3].x = 0.2f;
    transforms = MotionVectorGenerator::camera(current,previous);
    if (glm::length(MotionVectorGenerator::motionPixels(transforms,clip,{100,100})-glm::vec2(10,0)) > 1e-4f)
    {
        throw std::runtime_error("Camera motion must be previous-minus-current framebuffer pixels");
    }
}

// Stopped initialized Engine. false denotes unavailable formats/kernels, not pass.
inline bool runTemporalProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    runMotionVectorReferenceProbe();
    auto& device = *engine.device();
    TemporalConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    TemporalAccumulator temporal(configuration);
    temporal.initialize(device,engine.backendType(),2);
    if (!temporal.available())
    {
        temporal.cleanup();
        return false;
    }
    temporal.setMaxFrames(2);
    auto upload = [&](gpu::PixelFormat format, const void* data, size_t bytes)
    {
        gpu::ImageDescription description;
        description.size = {1,1};
        description.format = format;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst | gpu::ImageUsage::TransferSrc;
        auto image = device.createImage(description);
        image->upload({data,bytes,bytes,0,0});
        return image;
    };
    const uint16_t normalData[] = {0,0,0x3c00,0};
    const float positionData[] = {0,0,0,1};
    const uint8_t coverageData = 255, materialData[] = {0,0,255,0};
    auto normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,normalData,sizeof(normalData));
    auto position = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,positionData,sizeof(positionData));
    auto coverage = upload(gpu::PixelFormat::R8_UNORM,&coverageData,1);
    auto material = upload(gpu::PixelFormat::R8G8B8A8_UNORM,materialData,4);
    SurfaceInputs surface{SurfaceProfile::WorldPositionCoverage,normal.get(),position.get(),coverage.get()};
    TemporalView view;
    view.reproject = false;
    uint64_t sequence = 0, serial = 1, revision = 1, viewID = 0, layer = 0;
    using Half4 = std::array<uint16_t,4>;
    std::vector<std::shared_ptr<gpu::Image>> inputsRetained;
    auto render = [&](uint16_t visibility, Half4 diffuse, Half4 reflected)
    {
        auto ao = upload(gpu::PixelFormat::R16_SFLOAT,&visibility,2);
        auto gi = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,diffuse.data(),8);
        auto reflection = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,reflected.data(),8);
        inputsRetained.insert(inputsRetained.end(),{ao,gi,reflection});
        const uint32_t slot = uint32_t(sequence%2);
        serial += 7; // Presentation gaps do not imply missing rendered-scene samples.
        temporal.beginFrame(slot,serial,sequence);
        std::array<TemporalResult,3> outputs;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            const std::array<gpu::Image*,3> signals{ao.get(),gi.get(),reflection.get()};
            for (uint32_t kind = 0; kind < 3; ++kind)
            {
                outputs[kind] = temporal.record({*command,slot,serial,sequence,revision,
                    {TemporalSignal(kind),viewID,layer},*signals[kind],surface,view,kind == 2 ? material.get() : nullptr,0.75f});
            }
        });
        ++sequence;
        return outputs;
    };
    auto read = [&](const TemporalResult& result, uint32_t channels)
    {
        std::vector<uint8_t> bytes;
        result.image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != channels*2)
        {
            throw std::runtime_error("Temporal readback format/extent differs");
        }
        glm::vec4 value(0);
        for (uint32_t i = 0; i < channels; ++i)
        {
            uint16_t half;
            std::memcpy(&half,bytes.data()+i*2,2);
            const uint32_t exponent = (half>>10)&31u, mantissa = half&1023u;
            const float decoded = exponent ? std::ldexp(float(1024u+mantissa),int(exponent)-25) : std::ldexp(float(mantissa),-24);
            if (exponent == 31u)
            {
                throw std::runtime_error("Temporal output must be finite");
            }
            value[i] = (half&0x8000u) ? -decoded : decoded;
        }
        return value;
    };
    const Half4 red{0x4000,0,0,0x3c00}, blue{0,0,0x4000,0x3c00};
    auto first = render(0x3400,red,{0x4000,0,0,0x3400});
    auto second = render(0x3a00,blue,{0,0,0x4000,0x3a00});
    if (std::abs(read(second[0],1).r-0.5f) > 0.001f ||
        glm::length(read(second[1],4)-glm::vec4(1,0,1,1)) > 0.01f ||
        glm::length(read(second[2],4)-glm::vec4(0.5f,0,1.5f,0.5f)) > 0.01f || second[0].frameCount != 2)
    {
        throw std::runtime_error("Independent scalar/GI/confidence-weighted reflection accumulation differs");
    }
    if (std::abs(read(first[0],1).r-0.25f) > 0.001f)
    {
        throw std::runtime_error("Later slots overwrote retained previous temporal output");
    }
    auto third = render(0x3400,red,red);
    if (third[0].frameCount != 2 || std::abs(read(third[0],1).r-0.375f) > 0.001f)
    {
        throw std::runtime_error("Temporal rolling frame bound differs");
    }
    ++revision;
    auto reset = render(0x3a00,blue,blue);
    if (reset[0].frameCount != 1 || std::abs(read(reset[0],1).r-0.75f) > 0.001f)
    {
        throw std::runtime_error("External scene/reset generation must discard history");
    }
    const float movedPosition[] = {0,0,2,1};
    position = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,movedPosition,sizeof(movedPosition));
    surface.depthOrPosition = position.get();
    auto rejected = render(0x3400,red,red);
    if (std::abs(read(rejected[0],1).r-0.25f) > 0.001f)
    {
        throw std::runtime_error("World position rejection must restart pixel convergence");
    }
    const uint16_t oppositeNormal[] = {0,0,0xbc00,0};
    normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,oppositeNormal,sizeof(oppositeNormal));
    surface.normal = normal.get();
    auto rejectedNormal = render(0x3a00,blue,blue);
    if (std::abs(read(rejectedNormal[0],1).r-0.75f) > 0.001f)
    {
        throw std::runtime_error("Normal rejection must restart pixel convergence");
    }
    layer = 9;
    auto newLayer = render(0x3400,red,red);
    viewID = 2;
    auto newView = render(0x3a00,blue,blue);
    if (newLayer[0].frameCount != 1 || newView[0].frameCount != 1)
    {
        throw std::runtime_error("View/layer histories must not alias");
    }
    temporal.setFreezeAtLimit(true);
    render(0x3400,red,red);
    render(0x3a00,blue,blue);
    const auto frozen = render(0x3400,red,red);
    if (std::abs(read(frozen[0],1).r-0.5f) > 0.001f)
    {
        throw std::runtime_error("Frozen bounded convergence must retain the accepted frame mean");
    }
    const auto countBeforePause = temporal.frameCount({TemporalSignal::Visibility,viewID,layer});
    // No beginFrame/record for a UI-only presentation or paused view.
    serial += 100;
    if (temporal.frameCount({TemporalSignal::Visibility,viewID,layer}) != countBeforePause)
    {
        throw std::runtime_error("Presentation chronology must not advance scene accumulation");
    }
    // Exercise native camera-depth variants at the same identity receiver.
    const uint16_t encodedData[] = {0x3800,0x3800,0x3c00,0};
    auto encoded = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,encodedData,sizeof(encodedData));
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
    view.reproject = true;
    viewID = 3;
    render(0x3400,red,red);
    const auto camera = render(0x3a00,blue,blue);
    if (std::abs(read(camera[0],1).r-0.5f) > 0.001f)
    {
        throw std::runtime_error("Camera depth accumulation differs from explicit receivers");
    }
    temporal.setFreezeAtLimit(false);
    render(0x3400,red,red);
    const auto invalid = render(0x3a00,{0,0,0,0},{0,0,0,0});
    if (read(invalid[1],4) != glm::vec4(0) ||
        glm::length(read(invalid[2],4)-glm::vec4(2,0,0,0.5f)) > 0.01f)
    {
        throw std::runtime_error("Invalid GI and eligible reflection misses must use different history semantics");
    }
    const uint8_t roughData[] = {0,255,255,0};
    material = upload(gpu::PixelFormat::R8G8B8A8_UNORM,roughData,4);
    const auto rough = render(0x3400,red,red);
    if (read(rough[2],4) != glm::vec4(0))
    {
        throw std::runtime_error("Reflection roughness rejection must not revive previous hits");
    }
    ++revision;
    const auto black = render(0x3400,{0,0,0,0x3c00},red);
    if (read(black[1],4) != glm::vec4(0,0,0,1))
    {
        throw std::runtime_error("Valid black GI must retain alpha validity");
    }
    // Failed recording must not publish readable history, even when the next
    // attempt reuses the same scene sample number with a new recording ticket.
    const uint16_t retryValue = 0x3400;
    auto retrySignal = upload(gpu::PixelFormat::R16_SFLOAT,&retryValue,2);
    const TemporalKey retryKey{TemporalSignal::Visibility,999,999};
    const uint32_t retrySlot = uint32_t(sequence%2);
    temporal.beginFrame(retrySlot,++serial,sequence);
    auto abandoned = device.graphicsQueue().createCommandBuffer("abandoned temporal history");
    abandoned->begin();
    const auto abandonedResult = temporal.record({*abandoned,retrySlot,serial,sequence,revision,
        retryKey,*retrySignal,surface,view,nullptr,1});
    abandoned->end();
    abandoned.reset();
    temporal.beginFrame(retrySlot,++serial,sequence);
    TemporalResult retried;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        retried = temporal.record({*command,retrySlot,serial,sequence,revision,
            retryKey,*retrySignal,surface,view,nullptr,1});
    });
    if (retried.frameCount != 1 || retried.image == abandonedResult.image ||
        std::abs(read(retried,1).r-0.25f) > 0.001f)
    {
        throw std::runtime_error("Abandoned history or externally retained output was reused before safe retirement");
    }
    ++sequence;
    temporal.setEnabled(false);
    const auto disabled = render(0x3a00,red,red);
    if (disabled[0].owner || std::abs(read(disabled[0],1).r-0.75f) > 0.001f)
    {
        throw std::runtime_error("Disabled temporal pass must return the original borrowed signal");
    }
    temporal.cleanup();
    return true;
}
}
