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

// Observe recorded per-key state without GPU telemetry or a production debug API.
class TemporalWindowProbe : public pipeline::TemporalAccumulator {
  public:
    using TemporalAccumulator::TemporalAccumulator;

    uint32_t remaining(const pipeline::TemporalKey& key) const
    {
        const auto found = _histories.find(key);
        return found == _histories.end() ? 0 : found->second->rollingFramesRemaining;
    }
};

// Stopped initialized Engine. false denotes unavailable formats/kernels, not pass.
inline bool runTemporalProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    runMotionVectorReferenceProbe();
    auto& device = *engine.device();
    TemporalConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    TemporalWindowProbe temporal(configuration);
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
    auto render = [&](uint16_t visibility, Half4 diffuse, Half4 reflected, bool motion = false)
    {
        auto ao = upload(gpu::PixelFormat::R16_SFLOAT,&visibility,2);
        auto gi = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,diffuse.data(),8);
        auto reflection = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,reflected.data(),8);
        inputsRetained.insert(inputsRetained.end(),{ao,gi,reflection});
        const uint32_t slot = uint32_t(sequence%2);
        serial += 7; // Presentation gaps do not imply missing rendered-scene samples.
        temporal.beginFrame(slot,serial,sequence,motion);
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
    // At saturation, motion must update all three native signal kernels without
    // changing settings, generations, or previously returned image owners.
    const auto temporalGeneration = temporal.generation();
    const auto motion = render(0x3a00,blue,blue,true);
    if (std::abs(read(motion[0],1).r-0.625f) > 0.001f ||
        glm::length(read(motion[1],4)-glm::vec4(0.5f,0,1.5f,1)) > 0.01f ||
        glm::length(read(motion[2],4)-glm::vec4(0.5f,0,1.5f,1)) > 0.01f ||
        temporal.generation() != temporalGeneration || !temporal.settings().freezeAtLimit || temporal.settings().maxFrames != 2)
    {
        throw std::runtime_error("Motion must thaw scalar/GI/reflection kernels without invalidating authored settings");
    }
    for (uint32_t kind = 0; kind < 3; ++kind)
    {
        if (temporal.remaining({TemporalSignal(kind),viewID,layer}) != 6)
        {
            throw std::runtime_error("Every active signal must receive its own full settling window");
        }
    }
    float expectedVisibility = 0.625f;
    glm::vec4 expectedRadiance(0.5f,0,1.5f,1);
    for (uint32_t tail = 0; tail < 6; ++tail)
    {
        serial += 100; // No scene record occurs during this presentation gap.
        const auto settled = render(0x3400,red,red);
        expectedVisibility = (expectedVisibility+0.25f)*0.5f;
        expectedRadiance = (expectedRadiance+glm::vec4(2,0,0,1))*0.5f;
        if (std::abs(read(settled[0],1).r-expectedVisibility) > 0.002f ||
            glm::length(read(settled[1],4)-expectedRadiance) > 0.01f ||
            glm::length(read(settled[2],4)-expectedRadiance) > 0.01f ||
            temporal.remaining({TemporalSignal::Visibility,viewID,layer}) != 5-tail ||
            temporal.remaining({TemporalSignal::DiffuseRadiance,viewID,layer}) != 5-tail ||
            temporal.remaining({TemporalSignal::ReflectedRadiance,viewID,layer}) != 5-tail)
        {
            throw std::runtime_error("All six static tail records, including the last, must roll after presentation gaps");
        }
    }
    const auto refrozen = render(0x3a00,blue,blue);
    if (std::abs(read(refrozen[0],1).r-expectedVisibility) > 0.002f ||
        glm::length(read(refrozen[1],4)-expectedRadiance) > 0.01f ||
        glm::length(read(refrozen[2],4)-expectedRadiance) > 0.01f ||
        std::abs(read(frozen[0],1).r-0.5f) > 0.001f)
    {
        throw std::runtime_error("Seventh static record must refreeze without mutating retained output");
    }

    // Reuse two input images for longer state scenarios, including maxFrames=1024.
    // Native submissions/readbacks here are probe work, never ordinary frame telemetry.
    const uint16_t lowValue = 0x3400, highValue = 0x3a00;
    auto lowSignal = upload(gpu::PixelFormat::R16_SFLOAT,&lowValue,2);
    auto highSignal = upload(gpu::PixelFormat::R16_SFLOAT,&highValue,2);
    const TemporalKey windowA{TemporalSignal::Visibility,70,0};
    const TemporalKey windowB{TemporalSignal::Visibility,71,9};
    const auto windowFrame = [&](bool motionNow, bool includeB = true, bool high = false)
    {
        const uint32_t slot = uint32_t(sequence%2);
        temporal.beginFrame(slot,++serial,sequence,motionNow);
        std::array<TemporalResult,2> outputs;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            outputs[0] = temporal.record({*command,slot,serial,sequence,revision,windowA,
                high ? *highSignal : *lowSignal,surface,view});
            if (includeB)
            {
                outputs[1] = temporal.record({*command,slot,serial,sequence,revision,windowB,
                    high ? *highSignal : *lowSignal,surface,view});
            }
        });
        ++sequence;
        return outputs;
    };
    temporal.invalidate();
    windowFrame(true);
    windowFrame(false);
    windowFrame(true); // Renew a partially consumed window.
    if (temporal.remaining(windowA) != 6 || temporal.remaining(windowB) != 6)
    {
        throw std::runtime_error("Repeated motion must renew both independent key windows");
    }
    windowFrame(false,false);
    windowFrame(false);
    if (temporal.remaining(windowA) != 4 || temporal.remaining(windowB) != 0 || temporal.frameCount(windowB) != 1)
    {
        throw std::runtime_error("A skipped key must restart without changing another key's window");
    }
    const auto beforeUpdateOnly = temporal.remaining(windowA);
    temporal.beginFrame(uint32_t(sequence%2),++serial,sequence,true);
    if (temporal.remaining(windowA) != beforeUpdateOnly)
    {
        throw std::runtime_error("Begin without record must not consume or publish a settling window");
    }
    // Retry the unrecorded sequence: do not inherit its unrecorded motion flag.
    windowFrame(false);
    if (temporal.remaining(windowA) != beforeUpdateOnly-1)
    {
        throw std::runtime_error("Unrecorded motion must not renew the next recorded history");
    }
    temporal.beginFrame(uint32_t(sequence%2),++serial,sequence,true);
    ++sequence; // A skipped scene sequence breaks the predecessor chain.
    windowFrame(false);
    if (temporal.remaining(windowA) != 0 || temporal.frameCount(windowA) != 1)
    {
        throw std::runtime_error("Sequence gaps must discard old countdowns and unrecorded motion");
    }
    for (const uint32_t limit : {1u,1024u})
    {
        temporal.setMaxFrames(limit);
        windowFrame(true,false);
        if (temporal.remaining(windowA) != 3*limit)
        {
            throw std::runtime_error("Boundary frame limits must initialize the exact settling window");
        }
        for (uint32_t tail = 0; tail < 3*limit; ++tail)
        {
            const bool highTail = limit == 1 && tail%2 == 0;
            const auto tailOutput = windowFrame(false,false,highTail);
            if (limit == 1 && std::abs(read(tailOutput[0],1).r-(highTail ? 0.75f : 0.25f)) > 0.001f)
            {
                throw std::runtime_error("maxFrames=1 must replace the pixel on every rolling tail record");
            }
            if (temporal.remaining(windowA) != 3*limit-tail-1)
            {
                throw std::runtime_error("Boundary settling window countdown differs");
            }
        }
        const auto saturated = windowFrame(false,false,limit != 1);
        const float expectedFrozen = limit == 1 ? 0.75f : 0.25f;
        if (std::abs(read(saturated[0],1).r-expectedFrozen) > 0.0001f || saturated[0].frameCount != limit)
        {
            throw std::runtime_error("Boundary limit must refreeze after exactly three times the frame bound");
        }
    }
    temporal.setMaxFrames(2);
    temporal.setFreezeAtLimit(false);
    windowFrame(false);
    windowFrame(false,true,true);
    const auto rollingMotion = windowFrame(true);
    const auto rollingStatic = windowFrame(false,true,true);
    if (std::abs(read(rollingMotion[0],1).r-0.375f) > 0.001f ||
        std::abs(read(rollingStatic[0],1).r-0.5625f) > 0.001f)
    {
        throw std::runtime_error("Authored rolling mode must retain its bounded weighting during motion");
    }
    temporal.setFreezeAtLimit(true);
    windowFrame(true);
    temporal.invalidate();
    windowFrame(false);
    if (temporal.remaining(windowA) != 0 || temporal.frameCount(windowA) != 1)
    {
        throw std::runtime_error("Explicit invalidation must discard an active settling window");
    }
    windowFrame(true);
    ++revision;
    windowFrame(false);
    if (temporal.remaining(windowA) != 0 || temporal.frameCount(windowA) != 1)
    {
        throw std::runtime_error("External generation changes must discard an active settling window");
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
    temporal.setMaxFrames(1);
    temporal.setFreezeAtLimit(true);
    temporal.beginFrame(retrySlot,++serial,sequence,true);
    auto abandoned = device.graphicsQueue().createCommandBuffer("abandoned temporal history");
    abandoned->begin();
    const auto abandonedResult = temporal.record({*abandoned,retrySlot,serial,sequence,revision,
        retryKey,*retrySignal,surface,view,nullptr,1});
    abandoned->end();
    // Keep the first command alive but unsubmitted while retrying its sequence.
    temporal.beginFrame(retrySlot,++serial,sequence);
    TemporalResult retried;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        retried = temporal.record({*command,retrySlot,serial,sequence,revision,
            retryKey,*retrySignal,surface,view,nullptr,1});
    });
    if (retried.frameCount != 1 || retried.image == abandonedResult.image ||
        std::abs(read(retried,1).r-0.25f) > 0.001f || temporal.remaining(retryKey) != 0)
    {
        throw std::runtime_error("Abandoned history or externally retained output was reused before safe retirement");
    }
    abandoned.reset();
    ++sequence;
    temporal.beginFrame(uint32_t(sequence%2),++serial,sequence,true);
    auto discarded = device.graphicsQueue().createCommandBuffer("discarded motion window");
    discarded->begin();
    temporal.record({*discarded,uint32_t(sequence%2),serial,sequence,revision,
        retryKey,*retrySignal,surface,view});
    discarded->end();
    discarded.reset();
    ++sequence;
    temporal.beginFrame(uint32_t(sequence%2),++serial,sequence);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        retried = temporal.record({*command,uint32_t(sequence%2),serial,sequence,revision,
            retryKey,*highSignal,surface,view});
    });
    if (retried.frameCount != 1 || temporal.remaining(retryKey) != 0 || std::abs(read(retried,1).r-0.75f) > 0.001f)
    {
        throw std::runtime_error("Abandoned motion recording must not seed a successor countdown or signal");
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

// Separate opt-in probe: odd 3x3 receiver and 2x2 signal (ceil half resolution).
// Run only in a stopped initialized host; false means unavailable, never success.
inline bool runTemporalMetadataProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    auto& device = *engine.device();
    TemporalConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    configuration.metadataShaderInterface = "draw.temporal.v2";
    TemporalAccumulator temporal(configuration);
    temporal.initialize(device,engine.backendType(),2);
    if (!temporal.surfaceMetadataAvailable())
    {
        temporal.cleanup();
        return false;
    }
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
    std::array<uint16_t,36> normals{};
    for (size_t i = 0; i < 9; ++i)
    {
        normals[i*4] = normals[i*4+1] = 0x3800;
        normals[i*4+2] = 0x3c00;
    }
    auto normal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,normals.data(),sizeof(normals),24);
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
    auto receiver = depth(0.5f), mismatched = depth(0.75f);
    SurfaceInputs surface{SurfaceProfile::CameraDepth,normal.get(),receiver.get(),nullptr,glm::mat4(1)};
    TemporalView view;
    view.reproject = false;
    std::array<uint32_t,9> tokens;
    tokens.fill(7);
    uint64_t sequence = 0;
    std::shared_ptr<gpu::Image> diagnostic;
    auto render = [&](uint16_t value, const std::shared_ptr<gpu::Image>& metadataDepth)
    {
        const std::array<uint16_t,4> values{value,value,value,value};
        auto signal = upload({2,2},gpu::PixelFormat::R16_SFLOAT,values.data(),sizeof(values),4);
        auto token = upload({3,3},gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),12);
        const uint32_t slot = uint32_t(sequence%2);
        temporal.beginFrame(slot,sequence+1,sequence);
        TemporalResult result;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            result = temporal.record({*command,slot,sequence+1,sequence,1,{},*signal,surface,view,nullptr,1,
                {token,metadataDepth},true});
            diagnostic = temporal.recordDiagnostic(*command,slot,sequence+1,{},
                {token,metadataDepth},TemporalDiagnosticView::LocalHistory);
        });
        ++sequence;
        std::vector<uint8_t> bytes;
        result.image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != 8)
        {
            throw std::runtime_error("Odd-size metadata probe output extent differs");
        }
        std::array<uint16_t,4> output;
        std::memcpy(output.data(),bytes.data(),bytes.size());
        return output;
    };
    auto checkDiagnostic = [&](bool restarted, uint32_t x, uint32_t y)
    {
        if (!temporal.diagnosticsAvailable())
        {
            return; // Core v2 probe still works without the independent diagnostic kernel.
        }
        std::vector<uint8_t> bytes;
        diagnostic->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != 3*3*8)
        {
            throw std::runtime_error("Local-history diagnostic extent differs");
        }
        std::array<uint16_t,4> pixel;
        std::memcpy(pixel.data(),bytes.data()+(y*3+x)*8,8);
        const std::array<uint16_t,4> expected = restarted ? std::array<uint16_t,4>{0x3c00,0,0,0x3c00} :
            std::array<uint16_t,4>{0,0x3c00,0,0x3c00};
        if (pixel != expected)
        {
            throw std::runtime_error("Local-history diagnostic must distinguish restart from accepted history");
        }
    };
    render(0x3400,receiver); // .25
    checkDiagnostic(true,0,0);
    const auto accumulated = render(0x3a00,receiver); // .75 -> .5
    for (auto value : accumulated)
    {
        if (value != 0x3800)
        {
            throw std::runtime_error("Unchanged odd-size metadata must accumulate at signal texel centers");
        }
    }
    checkDiagnostic(false,0,0);
    tokens[0] = 8; // Signal pixel (0,0) maps to receiver (0,0); others to (2,0)/(0,2)/(2,2).
    const auto changed = render(0x3400,receiver);
    if (changed[0] != 0x3400 || changed[1] == 0x3400)
    {
        throw std::runtime_error("Token change must reject only the corresponding signal pixel");
    }
    checkDiagnostic(true,0,0);
    checkDiagnostic(false,2,0);
    const auto wrongDepth = render(0x3a00,mismatched);
    for (auto value : wrongDepth)
    {
        if (value != 0x3a00)
        {
            throw std::runtime_error("Metadata depth disagreement must restart each pixel with its raw signal");
        }
    }
    tokens.fill(0);
    const auto invalid = render(0x3400,receiver);
    if (invalid[0] != 0x3400)
    {
        throw std::runtime_error("Zero metadata token must reject history");
    }
    temporal.invalidate();
    tokens.fill(7);
    view.reproject = true;
    view.jitteredProjection[3].x = 4.0f;
    render(0x3400,receiver);
    view.jitteredProjection = glm::mat4(1);
    const auto outside = render(0x3a00,receiver);
    for (auto value : outside)
    {
        if (value != 0x3a00)
        {
            throw std::runtime_error("Out-of-bounds previous metadata must reject without edge clamping");
        }
    }
    // Diagnostic acceptance is independent of the bounded count, including maxFrames=1.
    temporal.setMaxFrames(1);
    view.reproject = false;
    render(0x3400,receiver);
    checkDiagnostic(true,0,0);
    render(0x3a00,receiver);
    checkDiagnostic(false,0,0);
    const std::array<uint16_t,4> samples{0x3400,0x3400,0x3400,0x3400};
    auto signal = upload({2,2},gpu::PixelFormat::R16_SFLOAT,samples.data(),sizeof(samples),4);
    auto validToken = upload({3,3},gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),12);
    const uint32_t oneToken = 7;
    auto wrongExtent = upload({1,1},gpu::PixelFormat::R32_UINT,&oneToken,sizeof(oneToken),4);
    const uint32_t slot = uint32_t(sequence%2);
    temporal.beginFrame(slot,sequence+1,sequence);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        const std::array<TemporalSurfaceMetadata,4> invalidPairs{{
            {validToken,{}},{normal,receiver},{wrongExtent,receiver},{validToken,normal}}};
        for (const auto& metadata : invalidPairs)
        {
            bool rejected = false;
            try
            {
                temporal.record({*command,slot,sequence+1,sequence,1,{TemporalSignal::Visibility,999,0},
                    *signal,surface,view,nullptr,1,metadata});
            }
            catch (const std::invalid_argument&)
            {
                rejected = true;
            }
            if (!rejected)
            {
                throw std::runtime_error("Malformed optional metadata must reject instead of silently accepting history");
            }
        }
    });
    temporal.cleanup();
    return true;
}
}
