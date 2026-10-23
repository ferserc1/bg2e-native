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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::tests {
// Stopped initialized host only; explicit manual execution, never ordinary-frame work.
// Returns false when the optional built-in v3/diagnostic family is unavailable.
inline bool runTemporalFootprintProbe(Engine& engine, const std::filesystem::path& shaderRoot, bool adaptive = false, bool moments = false)
{
    using namespace pipeline;
    auto& device = *engine.device();
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    TemporalConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    configuration.metadataShaderInterface = "draw.temporal.v2";
    configuration.reprojectionShaderInterface = "draw.temporal.v3";
    if (adaptive)
    {
        configuration.adaptiveShaderInterface = "draw.temporal.v4";
    }
    if (moments)
    {
        require(adaptive,"Moment fixture requires the adaptive variant");
        configuration.momentShaderInterface = "draw.temporal.v5";
    }
    class FootprintTemporalProbe : public TemporalAccumulator {
      public:
        using TemporalAccumulator::TemporalAccumulator;

        std::shared_ptr<gpu::Image> normalHistory() const
        {
            return _histories.at(TemporalKey{})->normal;
        }

        std::shared_ptr<gpu::Image> momentHistory() const
        {
            return _histories.at(TemporalKey{})->moments;
        }
    };
    FootprintTemporalProbe temporal(configuration);
    temporal.initialize(device,engine.backendType(),2);
    if (!temporal.surfaceReprojectionAvailable() || !temporal.diagnosticsAvailable() || (adaptive && !temporal.adaptiveAvailable()) || (moments && !temporal.momentsAvailable()))
    {
        temporal.cleanup();
        return false;
    }
    temporal.setPositionThresholds(2.0f,0);
    temporal.setAdaptiveSettings(adaptive,0,1,3);
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
    gpu::ImageDescription depthDescription;
    depthDescription.size = {3,3};
    depthDescription.format = gpu::PixelFormat::D32_SFLOAT;
    depthDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::DepthStencil;
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
    std::array<std::array<uint16_t,4>,9> normals, expectedNormals;
    normals.fill({0x3800,0x3800,0x3c00,0});
    expectedNormals.fill({0,0,0x3c00,0x3c00});
    auto normal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,normals.data(),sizeof(normals),24);
    std::array<glm::vec4,9> expected;
    expected.fill({0,0,0.5f,1}); // Reprojects every current texel to the four-tap midpoint.
    TemporalView view;
    view.view[3].z = -1;
    view.projection[3].z = 1;
    view.jitteredProjection = view.projection; // Identity VP, positive RH view depth.
    uint64_t sequence = 0;
    struct Output {
        TemporalResult signal;
        std::shared_ptr<gpu::Image> diagnostic, normalHistory, moments;
    };
    auto render = [&](const std::array<uint16_t,4>& values, const std::array<uint32_t,9>& tokens, bool sceneMotion = false)
    {
        auto signal = upload({2,2},gpu::PixelFormat::R16_SFLOAT,values.data(),sizeof(values),4);
        TemporalSurfaceMetadata metadata{upload({3,3},gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),12),depth};
        metadata.previousPosition = upload({3,3},gpu::PixelFormat::R32G32B32A32_SFLOAT,expected.data(),sizeof(expected),48);
        metadata.previousNormal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,expectedNormals.data(),sizeof(expectedNormals),24);
        metadata.legacyCompatible = false;
        SurfaceInputs surface{SurfaceProfile::CameraDepth,normal.get(),depth.get(),nullptr,glm::inverse(view.jitteredProjection*view.view)};
        const auto slot = uint32_t(sequence%2);
        temporal.beginFrame(slot,sequence+1,sequence,sceneMotion);
        Output output;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            output.signal = temporal.record({*command,slot,sequence+1,sequence,1,{},*signal,surface,view,nullptr,1,metadata,true});
            output.normalHistory = temporal.normalHistory();
            output.moments = temporal.momentHistory();
            output.diagnostic = temporal.recordDiagnostic(*command,slot,sequence+1,{},metadata,TemporalDiagnosticView::LocalHistory);
        });
        ++sequence;
        require(bool(output.diagnostic),"Optional v3 acceptance capture must be available for this fixture");
        return output;
    };
    auto read = [](gpu::Image& image)
    {
        std::vector<uint8_t> bytes;
        image.readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        std::vector<uint16_t> result(bytes.size()/2);
        std::memcpy(result.data(),bytes.data(),bytes.size());
        return result;
    };
    auto half = [](uint16_t value)
    {
        const auto exponent = int((value >> 10)&31);
        const float fraction = float(value & 1023u);
        const float magnitude = exponent == 0 ? std::ldexp(fraction,-24) :
            std::ldexp(1.0f+fraction/1024,exponent-15);
        return (value & 0x8000u) ? -magnitude : magnitude;
    };
    auto color = [&](const Output& output, const std::array<uint16_t,3>& expectedColor)
    {
        const auto pixels = read(*output.diagnostic);
        require(pixels.size() == 36,"Diagnostic image must preserve odd receiver extent");
        for (size_t pixel = 0; pixel < 9; ++pixel)
        {
            for (size_t channel = 0; channel < 3; ++channel)
            {
                require(pixels[pixel*4+channel] == expectedColor[channel],"Unexpected v3 footprint diagnostic reason");
            }
        }
    };
    std::array<uint32_t,9> currentTokens;
    currentTokens.fill(7);
    const std::array<uint16_t,4> oldValues{0,0x3400,0x3800,0x3c00}; // 0,.25,.5,1.
    const std::array<float,4> oldFloats{0,.25f,.5f,1};
    for (uint32_t mask = 0; mask < 16; ++mask)
    {
        temporal.invalidate();
        auto previousTokens = currentTokens;
        const std::array<size_t,4> centers{0,2,6,8};
        uint32_t count = 0;
        float sum = 0;
        for (uint32_t tap = 0; tap < 4; ++tap)
        {
            previousTokens[centers[tap]] = (mask & (1u << tap)) ? 7u : 8u;
            if (mask & (1u << tap))
            {
                ++count;
                sum += oldFloats[tap];
            }
        }
        const auto first = render(oldValues,previousTokens);
        const auto firstDiagnostic = read(*first.diagnostic);
        if (firstDiagnostic.size() >= 3 && firstDiagnostic[0] == 0x3c00 &&
            firstDiagnostic[1] == 0x3800 && firstDiagnostic[2] == 0)
        {
            temporal.cleanup();
            return false; // Optional capture unavailable: the display reports amber.
        }
        color(first,{0x3400,0x3400,0x3400}); // No predecessor, precedence over all other reasons.
        const auto second = render({0x3a00,0x3a00,0x3a00,0x3a00},currentTokens);
        const auto pixels = read(*second.signal.image);
        require(pixels.size() == 4,"Footprint signal must remain at reduced resolution");
        // V4 sees four matching constant-current neighbors: clipping resolves .75.
        const float reference = adaptive || !count ? 0.75f : (0.75f+sum/float(count))*0.5f;
        for (auto value : pixels)
        {
            require(std::abs(half(value)-reference) < 0.001f,"Validated scalar taps must renormalize without invalid colors");
        }
        if (moments)
        {
            require(bool(second.moments),"Selected v5 must retain a per-key moment image");
            const auto values = read(*second.moments);
            require(values.size() == 8,"Moments must be RG16F at reduced signal extent");
            float secondSum = 0;
            for (uint32_t tap = 0; tap < 4; ++tap)
            {
                if (mask & (1u << tap))
                {
                    secondSum += oldFloats[tap]*oldFloats[tap];
                }
            }
            const float oldMean = count ? sum/float(count) : 0.75f;
            const float delta = std::abs(oldMean-0.75f);
            const float reaction = std::clamp((delta-0.003f)/(delta+0.003f+1e-6f),0.0f,1.0f);
            const float alpha = count ? std::max({0.5f,1.0f-float(count)/4,reaction}) : 1.0f;
            const float mean = alpha*0.75f+(1-alpha)*oldMean;
            const float secondMoment = alpha*0.5625f+(1-alpha)*(count ? secondSum/float(count) : 0.5625f);
            for (size_t pixel = 0; pixel < 4; ++pixel)
            {
                require(std::abs(half(values[pixel*2])-mean) < 0.002f &&
                    std::abs(half(values[pixel*2+1])-secondMoment) < 0.002f,
                    "Moments must use exactly the valid color taps, normalized weights and effective alpha");
            }
        }
        if (moments)
        {
            const auto retained = read(*first.moments);
            require(retained.size() == 8,"Retained first moments must stay at signal extent");
            for (size_t pixel = 0; pixel < 4; ++pixel)
            {
                require(half(retained[pixel*2]) == oldFloats[pixel] &&
                    half(retained[pixel*2+1]) == oldFloats[pixel]*oldFloats[pixel],
                    "Retained moment owner must survive later history publication");
            }
        }
        color(second,count == 0 ? std::array<uint16_t,3>{0x3c00,0,0x3c00} :
            (count == 4 ? std::array<uint16_t,3>{0,0x3c00,0} : std::array<uint16_t,3>{0,0x3c00,0x3c00}));
    }
    if (adaptive)
    {
        temporal.invalidate();
        Output stable;
        for (uint32_t frame = 0; frame < 8; ++frame)
        {
            stable = render({0x3400,0x3400,0x3400,0x3400},currentTokens);
        }
        const auto stableCounts = read(*stable.normalHistory);
        require(stableCounts.size() == 16 && half(stableCounts[3]) == 8,
            "Stable adaptive samples must retain growing trust before a signal change");
        const auto changed = render({0x3a00,0x3a00,0x3a00,0x3a00},currentTokens);
        const auto changedCounts = read(*changed.normalHistory);
        require(changed.signal.frameCount == 9,
            "Nominal CPU frame count must stay separate from per-pixel adaptive trust");
        require(changedCounts.size() == 16,"Trust count guide must remain at signal extent");
        for (size_t pixel = 0; pixel < 4; ++pixel)
        {
            require(half(changedCounts[pixel*4+3]) == 2,
                "A reactive signal step must cap previous trust before incrementing once");
        }
        temporal.setMaxFrames(2);
        temporal.setFreezeAtLimit(true);
        render({0x3400,0x3400,0x3400,0x3400},currentTokens);
        render({0x3400,0x3400,0x3400,0x3400},currentTokens);
        const auto frozen = render({0x3a00,0x3a00,0x3a00,0x3a00},currentTokens);
        for (auto value : read(*frozen.signal.image))
        {
            require(value == 0x3400,"Literal saturated freeze must bypass neighborhood clipping/reactivity");
        }
        if (moments)
        {
            const auto frozenMoments = read(*frozen.moments);
            require(frozenMoments.size() == 8,"Frozen moments must retain signal extent");
            for (size_t pixel = 0; pixel < 4; ++pixel)
            {
                require(frozenMoments[pixel*2] == 0x3400 && frozenMoments[pixel*2+1] == 0x2c00,
                    "Frozen moments must preserve the previous .25/.0625 pair");
            }
        }
        const auto thawed = render({0x3a00,0x3a00,0x3a00,0x3a00},currentTokens,true);
        for (auto value : read(*thawed.signal.image))
        {
            require(value == 0x3a00,"Scene-motion rolling must permit the adaptive response without changing authored freeze");
        }
        require(temporal.settings().freezeAtLimit,"Adaptive rolling must preserve the authored freeze preference");
        temporal.setMaxFrames(32);
        temporal.setFreezeAtLimit(false);
    }
    // Same tokens, separate normal and position rejection, unsupported expected data,
    // off-screen projected domain and nonfinite current signal. First restart each pair.
    for (uint32_t scenario = 0; scenario < 8; ++scenario)
    {
        temporal.invalidate();
        render(oldValues,currentTokens);
        expected.fill({0,0,0.5f,1});
        expectedNormals.fill({0,0,0x3c00,0x3c00});
        if (scenario == 0)
        {
            expectedNormals.fill({0x3c00,0,0,0x3c00});
        }
        else if (scenario == 1)
        {
            temporal.setPositionThresholds(0.01f,0);
            // Setter invalidates: create predecessor under the new threshold first.
            render(oldValues,currentTokens);
        }
        else if (scenario == 2)
        {
            expected.fill({0,0,0.5f,0});
        }
        else if (scenario == 3)
        {
            expected.fill({4,0,0.5f,1});
        }
        if (scenario == 5)
        {
            normals.fill({0,0,0,0}); // Uncovered receiver, analogous to a transparent/cutout edge.
            normal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,normals.data(),sizeof(normals),24);
        }
        const uint16_t nonfinite = scenario == 6 ? 0x7c00 : (scenario == 7 ? 0xfc00 : 0x7e00);
        const auto values = scenario == 4 || scenario >= 6 ?
            std::array<uint16_t,4>{nonfinite,nonfinite,nonfinite,nonfinite} : oldValues;
        const auto output = render(values,currentTokens);
        const std::array<std::array<uint16_t,3>,8> colors{{{0x3c00,0x3c00,0},{0x3c00,0,0},
            {0x3800,0,0x3c00},{0,0x3400,0x3c00},{0x3c00,0x3c00,0x3c00},
            {0,0,0},{0x3c00,0x3c00,0x3c00},{0x3c00,0x3c00,0x3c00}}};
        color(output,colors[scenario]);
        normals.fill({0x3800,0x3800,0x3c00,0});
        normal = upload({3,3},gpu::PixelFormat::R16G16B16A16_SFLOAT,normals.data(),sizeof(normals),24);
        expected.fill({0,0,0.5f,1});
        expectedNormals.fill({0,0,0x3c00,0x3c00});
        temporal.setPositionThresholds(2.0f,0);
    }
    temporal.cleanup();
    return true;
}
}
