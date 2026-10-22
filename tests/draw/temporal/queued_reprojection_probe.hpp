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
#include <bg2e/gpu/Queue.hpp>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include <utility>

namespace bg2e::draw::tests {
inline float temporalProbeHalf(uint16_t value)
{
    const auto exponent = (value >> 10)&31u;
    const auto fraction = value&1023u;
    float result;
    if (exponent == 0)
    {
        result = std::ldexp(float(fraction),-24);
    }
    else if (exponent == 31)
    {
        result = fraction ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity();
    }
    else
    {
        result = std::ldexp(float(1024+fraction),int(exponent)-25);
    }
    return value&0x8000u ? -result : result;
}

// Stopped initialized host. Native source fixture only; no SDK/RT required.
// First two/three submissions occur before a fixture wait/readback, but GPU overlap
// is not guaranteed. All inputs are uploaded before those submission batches.
inline bool runQueuedTemporalReprojectionProbe(Engine& engine, const std::filesystem::path& shaderRoot)
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
    const auto uploadUsage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    for (const auto format : {gpu::PixelFormat::R16_SFLOAT,gpu::PixelFormat::R16G16_SFLOAT,
        gpu::PixelFormat::R16G16B16A16_SFLOAT,gpu::PixelFormat::R32G32B32A32_SFLOAT,gpu::PixelFormat::R32_UINT})
    {
        if (!device.formatCapabilities(format).supports(uploadUsage))
        {
            return false;
        }
    }
    auto upload = [&](gpu::Size2D size, gpu::PixelFormat format, const void* data, size_t bytes, size_t row)
    {
        gpu::ImageDescription description;
        description.size = size;
        description.format = format;
        description.usage = uploadUsage;
        auto image = device.createImage(description);
        image->upload({data,bytes,row,0,0});
        return image;
    };
    for (const uint32_t slots : {2u,3u})
    {
        TemporalConfiguration configuration;
        configuration.shaderRoot = shaderRoot;
        configuration.metadataShaderInterface = "draw.temporal.v2";
        configuration.reprojectionShaderInterface = "draw.temporal.v3";
        TemporalAccumulator temporal(configuration);
        temporal.initialize(device,engine.backendType(),slots);
        if (!temporal.surfaceReprojectionAvailable() || !temporal.surfaceDiagnosticsAvailable())
        {
            temporal.cleanup();
            return false;
        }
        temporal.setPositionThresholds(0.001f,0);
        temporal.setMaxFrames(4);
        temporal.setFreezeAtLimit(true);
        gpu::ImageDescription depthDescription;
        depthDescription.size = {3,3};
        depthDescription.format = gpu::PixelFormat::D32_SFLOAT;
        depthDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::DepthStencil;
        if (!device.formatCapabilities(depthDescription.format).supports(depthDescription.usage))
        {
            temporal.cleanup();
            return false;
        }
        auto depth = device.createImage(depthDescription);
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(depth.get(),gpu::ImageLayout::DepthAttachment);
            gpu::RenderingDescription rendering;
            rendering.depth = gpu::DepthAttachmentDescription{depth.get(),gpu::AttachmentLoadOperation::Clear,
                gpu::AttachmentStoreOperation::Store,0.5f};
            command->beginRendering(rendering);
            command->endRendering();
            command->transition(depth.get(),gpu::ImageLayout::ShaderReadOnly);
        });
        std::array<std::array<uint16_t,4>,9> normalValues, materialValues, previousNormals;
        normalValues.fill({0x3800,0x3800,0x3c00,0});
        materialValues.fill({0,0x3800,0,0});
        previousNormals.fill({0,0,0x3c00,0x3c00});
        auto normal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,normalValues.data(),sizeof(normalValues),24);
        auto material = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,materialValues.data(),sizeof(materialValues),24);
        struct Inputs {
            TemporalView view;
            TemporalSurfaceMetadata metadata;
            std::array<std::shared_ptr<gpu::Image>,3> signals;
        };
        const uint64_t baselineCount = 2*slots;
        std::vector<Inputs> inputs;
        for (uint64_t scene = 0; scene < baselineCount+6; ++scene)
        {
            Inputs input;
            input.view.view[3].x = -0.02f*float(scene);
            std::array<uint32_t,9> tokens;
            tokens.fill(scene == baselineCount+4 ? 9u : (scene == baselineCount+5 ? 0u : 7u));
            input.metadata.token = upload({3,3},gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),12);
            input.metadata.depth = depth;
            input.metadata.legacyCompatible = false;
            std::array<glm::vec4,9> positions;
            for (uint32_t y = 0; y < 3; ++y)
            {
                for (uint32_t x = 0; x < 3; ++x)
                {
                    positions[y*3+x] = {2*(float(x)+0.5f)/3-1+0.02f*float(scene ? scene-1 : 0),
                        1-2*(float(y)+0.5f)/3,0.5f,1};
                }
            }
            input.metadata.previousPosition = upload({3,3},gpu::PixelFormat::R32G32B32A32_SFLOAT,positions.data(),sizeof(positions),48);
            input.metadata.previousNormal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,previousNormals.data(),sizeof(previousNormals),24);
            std::array<std::array<uint16_t,2>,9> motion;
            motion.fill({0x3800,0xb400}); // Synthetic display oracle (.5,-.25), independent of receiver transport.
            input.metadata.motion = upload({3,3},gpu::PixelFormat::R16G16_SFLOAT,motion.data(),sizeof(motion),12);
            std::array<uint16_t,4> visibility;
            visibility.fill(scene ? 0x3a00 : 0x3400);
            input.signals[0] = upload({2,2},gpu::PixelFormat::R16_SFLOAT,visibility.data(),sizeof(visibility),4);
            std::array<std::array<uint16_t,4>,4> diffuse, reflected;
            diffuse.fill(scene ? std::array<uint16_t,4>{0x3a00,0x3a00,0x3a00,0x3c00} :
                std::array<uint16_t,4>{0,0,0,0x3c00}); // Valid black GI remains covered.
            reflected.fill(scene ? std::array<uint16_t,4>{0x3a00,0x3a00,0x3a00,0x3a00} :
                std::array<uint16_t,4>{0x3400,0x3400,0x3400,0x3400});
            input.signals[1] = upload({2,2},gpu::PixelFormat::R16G16B16A16_SFLOAT,diffuse.data(),sizeof(diffuse),16);
            input.signals[2] = upload({2,2},gpu::PixelFormat::R16G16B16A16_SFLOAT,reflected.data(),sizeof(reflected),16);
            inputs.push_back(std::move(input));
        }
        struct Recorded {
            std::array<TemporalResult,3> signals;
            std::array<std::shared_ptr<gpu::Image>,3> displays;
        };
        std::vector<std::shared_ptr<gpu::CommandBuffer>> commands;
        auto record = [&](uint64_t scene, bool submit)
        {
            const uint32_t slot = uint32_t(scene%slots);
            auto& input = inputs.at(size_t(scene));
            temporal.beginFrame(slot,scene+1,scene,scene == 1);
            auto command = device.graphicsQueue().createCommandBuffer("queued v3 source fixture");
            command->begin();
            Recorded result;
            for (uint32_t signal = 0; signal < 3; ++signal)
            {
                SurfaceInputs surface{SurfaceProfile::CameraDepth,normal.get(),depth.get(),nullptr,glm::inverse(input.view.view)};
                result.signals[signal] = temporal.record({*command,slot,scene+1,scene,1,{TemporalSignal(signal),0,0},
                    *input.signals[signal],surface,input.view,signal == 2 ? material.get() : nullptr,1,input.metadata});
            }
            if (scene == 1 || scene == baselineCount+5)
            {
                auto missingMotion = input.metadata;
                missingMotion.motion.reset();
                require(!temporal.recordDiagnostic(*command,slot,scene+1,{},missingMotion,TemporalDiagnosticView::Motion),
                    "Missing motion display input must report unavailable rather than valid zero motion");
                auto wrongMotion = input.metadata;
                wrongMotion.motion = input.metadata.previousPosition;
                bool rejected = false;
                try
                {
                    temporal.recordDiagnostic(*command,slot,scene+1,{},wrongMotion,TemporalDiagnosticView::Motion);
                }
                catch (const std::invalid_argument&)
                {
                    rejected = true;
                }
                require(rejected,"A motion display must not bind previous-position data as RG16F motion");
                const std::array<TemporalDiagnosticView,3> views{TemporalDiagnosticView::Motion,
                    TemporalDiagnosticView::PreviousPosition,TemporalDiagnosticView::PreviousNormal};
                for (size_t i = 0; i < views.size(); ++i)
                {
                    result.displays[i] = temporal.recordDiagnostic(*command,slot,scene+1,{},input.metadata,views[i]);
                    require(bool(result.displays[i]),"Supported surface diagnostics must expose every Phase 2 view");
                }
            }
            command->end();
            if (submit)
            {
                device.graphicsQueue().submit(command.get());
                commands.push_back(command);
            }
            else
            {
                command.reset(); // Abandons only unsubmitted wrapper; result owners prevent unsafe scratch reuse.
            }
            return result;
        };
        std::vector<Recorded> baseline;
        for (uint64_t scene = 0; scene < baselineCount; ++scene)
        {
            if (scene && scene%slots == 0)
            {
                device.waitIdle(); // Fixture-only acquisition: retire slots before reuse, never before the first batch.
            }
            baseline.push_back(record(scene,true));
        }
        device.waitIdle();
        auto read = [&](const std::shared_ptr<gpu::Image>& image)
        {
            std::vector<uint8_t> bytes;
            image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
            require(bytes.size()%2 == 0,"Half-float fixture readback must contain complete components");
            std::vector<float> values;
            for (size_t i = 0; i < bytes.size(); i += 2)
            {
                uint16_t value;
                std::memcpy(&value,bytes.data()+i,2);
                values.push_back(temporalProbeHalf(value));
            }
            return values;
        };
        float visibility = 0.25f, diffuse = 0, confidence = 0.25f, reflected = 0.25f;
        for (size_t scene = 0; scene < baseline.size(); ++scene)
        {
            if (scene)
            {
                const float weight = 1-1.0f/float(std::min(size_t(4),scene+1));
                visibility = visibility*weight+0.75f*(1-weight);
                diffuse = diffuse*weight+0.75f*(1-weight);
                const float nextConfidence = confidence*weight+0.75f*(1-weight);
                reflected = (reflected*confidence*weight+0.75f*0.75f*(1-weight))/nextConfidence;
                confidence = nextConfidence;
            }
            for (uint32_t signal = 0; signal < 3; ++signal)
            {
                require(baseline[scene].signals[signal].frameCount == std::min(uint32_t(4),uint32_t(scene+1)),
                    "Queued moving cameras must preserve bounded per-key chronology");
                const auto values = read(baseline[scene].signals[signal].owner);
                const uint32_t components = signal ? 4u : 1u;
                require(values.size() == 4*components,"Queued v3 output must retain reduced 2x2 signal size");
                const float expected = signal == 0 ? visibility : (signal == 1 ? diffuse : reflected);
                for (size_t pixel = 0; pixel < 4; ++pixel)
                {
                    require(std::isfinite(values[pixel*components]) && std::abs(values[pixel*components]-expected) < 0.003f,
                        "Queued v3 must transport scalar/GI/confidence-weighted reflection history across camera/object movement");
                    if (signal)
                    {
                        require(std::abs(values[pixel*4+3]-(signal == 1 ? 1.0f : confidence)) < 0.003f,
                            "V3 must preserve valid-black GI and separately accumulated reflection confidence");
                    }
                }
            }
        }
        const auto motion = read(baseline[1].displays[0]);
        const auto position = read(baseline[1].displays[1]);
        const auto previousNormal = read(baseline[1].displays[2]);
        require(motion.size() == 36 && position.size() == 36 && previousNormal.size() == 36,
            "Surface diagnostic owners must retain receiver extent across slot reuse");
        require(std::abs(motion[0]-0.5078125f) < 0.001f && std::abs(motion[1]-0.49609375f) < 0.001f &&
            motion[2] == 0.5f && previousNormal[0] == 0.5f && previousNormal[1] == 0.5f && previousNormal[2] == 1,
            "Motion and expected-normal displays must preserve the documented signed mapping");
        const float expectedX = 0.5f+0.5f*(-2.0f/3)/(1+2.0f/3);
        require(std::abs(position[0]-expectedX) < 0.001f,
            "Expected-position display must use the retained previous world point");
        const auto abandoned = record(baselineCount,false);
        const auto afterAbandon = record(baselineCount+1,true);
        device.waitIdle();
        for (const auto& signal : afterAbandon.signals)
        {
            require(signal.frameCount == 1,"Abandoned predecessor must not supply queued temporal history");
            require(std::abs(read(signal.owner)[0]-0.75f) < 0.001f,"Abandoned history must restart from the raw signal");
        }
        temporal.beginFrame(uint32_t((baselineCount+2)%slots),baselineCount+3,baselineCount+2); // Update-only gap.
        const auto afterUpdateOnly = record(baselineCount+3,true);
        device.waitIdle();
        require(afterUpdateOnly.signals[0].frameCount == 1,"An update-only scene cannot bridge a missing rendered contribution");
        const auto newToken = record(baselineCount+4,true);
        device.waitIdle();
        require(newToken.signals[0].frameCount == 2 && std::abs(read(newToken.signals[0].owner)[0]-0.75f) < 0.001f,
            "New tokens must reject locally without pretending the nominal key count reset globally");
        const auto unsupported = record(baselineCount+5,true);
        device.waitIdle();
        require(std::abs(read(unsupported.signals[0].owner)[0]-0.75f) < 0.001f,
            "Unsupported/zero-token correspondence must restart the local raw sample");
        for (const auto& display : unsupported.displays)
        {
            const auto values = read(display);
            for (size_t pixel = 0; pixel < 9; ++pixel)
            {
                require(values[pixel*4] == 0 && values[pixel*4+1] == 0 && values[pixel*4+2] == 0 && values[pixel*4+3] == 1,
                    "Zero-token surface diagnostics must remain black instead of showing stale geometry/motion");
            }
        }
        // Keep abandoned result images alive until cleanup: their recorded transitions never executed.
        require(bool(abandoned.signals[0].owner) && read(baseline[0].signals[1].owner)[0] == 0,
            "Retained first valid-black and abandoned owners must survive all queued-slot reuse");
        temporal.cleanup();
    }
    return true;
}
}
