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
#include "queued_reprojection_probe.hpp"

namespace bg2e::draw::tests {
// Stopped initialized host only. Synthetic shader fixture, no RT/FSR or visual claim.
// forceMomentFailure selects an intentionally missing optional family, not a fake device capability.
inline bool runAdaptiveTemporalProbe(Engine& engine, const std::filesystem::path& shaderRoot,
    bool moments = false, bool forceMomentFailure = false, bool metrics = false)
{
    using namespace pipeline;
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    require(!moments || !forceMomentFailure,"Choose supported moments or forced optional failure separately");
    auto& device = *engine.device();
    TemporalConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    configuration.metadataShaderInterface = "draw.temporal.v2";
    configuration.reprojectionShaderInterface = "draw.temporal.v3";
    configuration.adaptiveShaderInterface = "draw.temporal.v4";
    if (moments || forceMomentFailure)
    {
        configuration.momentShaderInterface = "draw.temporal.v5";
        if (forceMomentFailure)
        {
            configuration.momentShaderPrefix = "tests/intentionally_missing_temporal_moments";
        }
    }
    class AdaptiveProbe : public TemporalAccumulator {
      public:
        using TemporalAccumulator::TemporalAccumulator;

        std::shared_ptr<gpu::Image> guide(const TemporalKey& key, bool moment) const
        {
            return moment ? _histories.at(key)->moments : _histories.at(key)->normal;
        }

        std::shared_ptr<gpu::Image> metricHistory(const TemporalKey& key) const
        {
            return _histories.at(key)->metrics;
        }
    };
    AdaptiveProbe temporal(configuration), baseline(configuration);
    temporal.initialize(device,engine.backendType(),2);
    baseline.initialize(device,engine.backendType(),2);
    if (!temporal.adaptiveAvailable() || !baseline.surfaceReprojectionAvailable() || (moments && !temporal.momentsAvailable()))
    {
        temporal.cleanup();
        baseline.cleanup();
        return false;
    }
    if (forceMomentFailure)
    {
        require(!temporal.momentsAvailable() && temporal.momentsStatus().find("neighborhood-only v4 fallback") != std::string::npos,
            "An unavailable optional moment family must retain explicit v4 fallback");
    }
    if (metrics && !temporal.adaptiveDiagnosticsAvailable())
    {
        temporal.cleanup();
        baseline.cleanup();
        return false;
    }
    std::unique_ptr<AdaptiveProbe> control;
    if (metrics)
    {
        control = std::make_unique<AdaptiveProbe>(configuration);
        control->initialize(device,engine.backendType(),2);
        if (!control->adaptiveAvailable() || (moments && !control->momentsAvailable()))
        {
            temporal.cleanup();
            baseline.cleanup();
            control->cleanup();
            return false;
        }
        control->setAdaptiveSettings(true,0.03125f,0.25f,3);
        control->setPositionThresholds(2,0);
    }
    temporal.setAdaptiveSettings(true,0.03125f,0.25f,3);
    temporal.setPositionThresholds(2,0);
    baseline.setPositionThresholds(2,0);
    auto upload = [&](gpu::PixelFormat format, const void* data, size_t bytes, size_t row)
    {
        gpu::ImageDescription description;
        description.size = {3,3};
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
    using Pixel = std::array<uint16_t,4>;
    using Pixels = std::array<Pixel,9>;
    const auto uniform = [](Pixel value)
    {
        Pixels values;
        values.fill(value);
        return values;
    };
    const auto encoded = uniform({0x3800,0x3800,0x3c00,0});
    const auto expectedNormals = uniform({0,0,0x3c00,0x3c00});
    const auto materials = uniform({0,0x3800,0,0});
    auto normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,encoded.data(),sizeof(encoded),24);
    auto expectedNormal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,expectedNormals.data(),sizeof(expectedNormals),24);
    auto material = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,materials.data(),sizeof(materials),24);
    std::array<glm::vec4,9> positions;
    for (uint32_t y = 0; y < 3; ++y)
    {
        for (uint32_t x = 0; x < 3; ++x)
        {
            positions[y*3+x] = {2*(float(x)+0.5f)/3-1,1-2*(float(y)+0.5f)/3,0.5f,1};
        }
    }
    auto expectedPosition = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,positions.data(),sizeof(positions),48);
    TemporalView view;
    view.view[3].z = -1;
    view.projection[3].z = 1;
    view.jitteredProjection = view.projection;
    SurfaceInputs surface{SurfaceProfile::CameraDepth,normal.get(),depth.get(),nullptr,glm::inverse(view.jitteredProjection*view.view)};
    struct Output {
        TemporalResult signal;
        std::shared_ptr<gpu::Image> normal, moments, metrics;
        std::array<std::shared_ptr<gpu::Image>,4> displays;
    };
    auto read = [&](const std::shared_ptr<gpu::Image>& image)
    {
        std::vector<uint8_t> bytes;
        image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        require(bytes.size()%2 == 0,"Half readback requires full components");
        std::vector<float> values;
        for (size_t i = 0; i < bytes.size(); i += 2)
        {
            uint16_t value;
            std::memcpy(&value,bytes.data()+i,2);
            values.push_back(temporalProbeHalf(value));
        }
        return values;
    };
    uint64_t sequence = 0;
    auto render = [&](TemporalSignal kind, const Pixels& pixels, bool isolateCenter = false, bool sceneMotion = false, bool suppressCapture = false)
    {
        std::array<uint16_t,9> scalars;
        for (size_t i = 0; i < 9; ++i)
        {
            scalars[i] = pixels[i][0];
        }
        auto signal = kind == TemporalSignal::Visibility ?
            upload(gpu::PixelFormat::R16_SFLOAT,scalars.data(),sizeof(scalars),6) :
            upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,pixels.data(),sizeof(pixels),24);
        std::array<uint32_t,9> tokens;
        tokens.fill(isolateCenter ? 8u : 7u);
        tokens[4] = 7;
        TemporalSurfaceMetadata metadata{upload(gpu::PixelFormat::R32_UINT,tokens.data(),sizeof(tokens),12),depth};
        metadata.previousPosition = expectedPosition;
        metadata.previousNormal = expectedNormal;
        metadata.legacyCompatible = false;
        const uint32_t slot = uint32_t(sequence%2);
        std::array<AdaptiveProbe*,3> accumulators{&temporal,&baseline,control.get()};
        std::array<Output,3> result;
        for (auto* accumulator : accumulators)
        {
            if (accumulator)
            {
                accumulator->beginFrame(slot,sequence+1,sequence,sceneMotion);
            }
        }
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            for (size_t i = 0; i < accumulators.size(); ++i)
            {
                if (!accumulators[i])
                {
                    continue;
                }
                if (metrics && i == 0 && sequence == 0)
                {
                    bool rejected = false;
                    try
                    {
                        accumulators[i]->record({*command,slot,sequence+1,sequence,1,{kind,0,0},*signal,surface,view,
                            kind == TemporalSignal::ReflectedRadiance ? material.get() : nullptr,1,metadata,true,true});
                    }
                    catch (const std::invalid_argument&)
                    {
                        rejected = true;
                    }
                    require(rejected,"Mutually exclusive capture requests must reject before recording history");
                }
                result[i].signal = accumulators[i]->record({*command,slot,sequence+1,sequence,1,{kind,0,0},
                    *signal,surface,view,kind == TemporalSignal::ReflectedRadiance ? material.get() : nullptr,1,metadata,false,metrics && i < 2 && !suppressCapture});
                result[i].normal = accumulators[i]->guide({kind,0,0},false);
                result[i].moments = accumulators[i]->guide({kind,0,0},true);
                result[i].metrics = accumulators[i]->metricHistory({kind,0,0});
                if (metrics && i < 2)
                {
                    for (uint32_t channel = 0; channel < 4; ++channel)
                    {
                        result[i].displays[channel] = accumulators[i]->recordDiagnostic(*command,slot,sequence+1,
                            {kind,0,0},metadata,TemporalDiagnosticView(uint32_t(TemporalDiagnosticView::CurrentWeight)+channel));
                    }
                }
            }
        });
        ++sequence;
        require(!result[1].moments,"Disabled adaptation must not allocate moment history even if v5 is provisioned");
        require(bool(result[0].moments) == moments,"Unavailable/disabled moments must select the v4 path");
        if (metrics && result[0].metrics)
        {
            require(!result[1].metrics && !result[2].metrics,"Disabled/uncaptured paths must not allocate metric images");
            const auto captured = read(result[0].metrics);
            const auto normalGuide = read(result[0].normal);
            require(captured.size() == 36,"Metrics must be RGBA16F at signal resolution");
            for (size_t component = 0; component < captured.size(); ++component)
            {
                require(std::isfinite(captured[component]) && captured[component] >= 0 && captured[component] <= 1,
                    "Diagnostic encodings must stay finite in [0,1]");
            }
            for (size_t pixel = 0; pixel < 9; ++pixel)
            {
                require(std::abs(captured[pixel*4+3]-normalGuide[pixel*4+3]/float(temporal.settings().maxFrames)) < 0.002f,
                    "Effective count view must reflect post-response trust, not CPU nominal frames");
            }
            for (uint32_t channel = 0; channel < 4; ++channel)
            {
                require(bool(result[0].displays[channel]) && bool(result[1].displays[channel]),"Available metric display must produce an image");
                const auto display = read(result[0].displays[channel]);
                const auto unavailable = read(result[1].displays[channel]);
                require(display.size() == 36 && unavailable.size() == 36,"Metric displays must retain receiver resolution");
                for (size_t pixel = 0; pixel < 9; ++pixel)
                {
                    for (size_t rgb = 0; rgb < 3; ++rgb)
                    {
                        require(std::abs(display[pixel*4+rgb]-captured[pixel*4+channel]) < 0.002f,
                            "Selected view must show the correct captured channel");
                    }
                    require(unavailable[pixel*4] == 1 && unavailable[pixel*4+1] == 0.5f && unavailable[pixel*4+2] == 0,
                        "Disabled adaptation must display missing-capture amber instead of fabricated metrics");
                }
            }
            for (const auto& pair : {std::pair{result[0].signal.owner,result[2].signal.owner},
                std::pair{result[0].normal,result[2].normal},std::pair{result[0].moments,result[2].moments}})
            {
                if (!pair.first)
                {
                    require(!pair.second,"Diagnostic control must use the same moment mode");
                    continue;
                }
                const auto diagnosed = read(pair.first), ordinary = read(pair.second);
                require(diagnosed.size() == ordinary.size(),"Capture must preserve output formats/extents");
                for (size_t component = 0; component < diagnosed.size(); ++component)
                {
                    require(std::abs(diagnosed[component]-ordinary[component]) <= std::max(0.003f,std::abs(ordinary[component])*0.002f),
                        "Metric capture must preserve ordinary color/guide/moments within FP16 tolerance");
                }
            }
        }
        return result;
    };
    const auto reset = [&]
    {
        temporal.invalidate();
        baseline.invalidate();
        if (control)
        {
            control->invalidate();
        }
    };
    const auto finite = [&](const Output& output)
    {
        for (float value : read(output.signal.owner))
        {
            require(std::isfinite(value) && std::abs(value) <= 65504,"Adaptive output must remain finite FP16");
        }
        if (output.moments)
        {
            const auto values = read(output.moments);
            require(values.size() == 18,"Moment output must stay RG16F at signal resolution");
            for (size_t pixel = 0; pixel < 9; ++pixel)
            {
                require(std::isfinite(values[pixel*2]) && values[pixel*2] >= 0 && values[pixel*2] <= 16 &&
                    std::isfinite(values[pixel*2+1]) && values[pixel*2+1] >= 0 && values[pixel*2+1] <= 256,
                    "HDR/scalar moments must stay finite within bounded statistic limits");
            }
        }
    };
    const auto low = uniform({0x3400,0,0,0});
    const auto high = uniform({0x3a00,0,0,0});
    for (uint32_t frame = 0; frame < 8; ++frame)
    {
        const auto initial = render(TemporalSignal::Visibility,low);
        if (metrics && !initial[0].metrics)
        {
            temporal.cleanup();
            baseline.cleanup();
            control->cleanup();
            return false; // Exact selected v4/v5 capture family can be unavailable independently.
        }
        if (metrics && frame == 0)
        {
            const auto values = read(initial[0].metrics);
            require(values[0] == 1 && values[1] == 0 && values[2] == 0 && values[3] == 0.03125f,
                "Cold valid sample must report alpha one/reactivity zero/unevaluated variance/count one");
        }
    }
    const auto generation = temporal.generation();
    const auto step = render(TemporalSignal::Visibility,high);
    require(temporal.generation() == generation,"Signal changes must not invoke accumulator-global resets");
    const auto adaptiveStep = read(step[0].signal.owner), baselineStep = read(step[1].signal.owner);
    require(std::abs(adaptiveStep[4]-0.75f) < 0.002f &&
        std::abs(adaptiveStep[4]-0.75f) < std::abs(baselineStep[4]-0.75f),
        "A controlled stationary scalar step must react faster than the identical nonadaptive stream");
    require(read(step[0].normal)[4*4+3] < 9 && step[0].signal.frameCount == 9,
        "Effective pixel trust must be distinct from nominal consecutive chronology");
    if (metrics)
    {
        const auto uncaptured = render(TemporalSignal::Visibility,high,false,false,true);
        require(!uncaptured[0].metrics,"Suppressing capture must not allocate an unused signal metric image");
        const auto resumed = render(TemporalSignal::Visibility,high);
        require(resumed[0].signal.frameCount == 11 && read(resumed[0].metrics)[4*4] < 1,
            "Switching metric pipelines must preserve accepted chronology instead of restarting history");
    }
    reset();
    render(TemporalSignal::Visibility,uniform({0,0,0,0}),true);
    const auto single = render(TemporalSignal::Visibility,uniform({0x3c00,0,0,0}),true);
    require(std::abs(read(single[0].signal.owner)[4]-0.5f) < 0.002f,
        "One valid same-token neighbor must disable variance clipping/reactivity");
    reset();
    for (uint32_t frame = 0; frame < 24; ++frame)
    {
        Pixels noise;
        for (size_t pixel = 0; pixel < 9; ++pixel)
        {
            noise[pixel] = {(frame+pixel)%2 ? uint16_t(0x3a00) : uint16_t(0x3400),0,0,0};
        }
        const auto output = render(TemporalSignal::Visibility,noise);
        finite(output[0]);
        require(output[0].signal.frameCount == frame+1,"Noise must keep nominal chronology without per-frame resets");
        if (frame == 23)
        {
            require(std::abs(read(output[0].signal.owner)[4]-0.5f) < 0.01f,
                "A deterministic balanced noisy stream must retain a bounded converged center");
        }
    }
    reset();
    const auto black = uniform({0,0,0,0x3c00});
    const auto blackResult = render(TemporalSignal::DiffuseRadiance,black);
    render(TemporalSignal::DiffuseRadiance,black);
    const auto blackPixels = read(blackResult[0].signal.owner);
    require(blackPixels[0] == 0 && blackPixels[1] == 0 && blackPixels[2] == 0 && blackPixels[3] == 1,
        "Valid-black GI must remain covered and distinct from missing illumination");
    const auto hdr = uniform({0x7bff,0x7bff,0x7bff,0x3c00});
    const auto bright = render(TemporalSignal::DiffuseRadiance,hdr);
    finite(bright[0]);
    require(read(bright[0].signal.owner)[0] > 65000,"Near-black HDR fallback must not overflow while clipping upward");
    reset();
    render(TemporalSignal::DiffuseRadiance,black);
    auto outlier = black;
    outlier[4] = {0x7bff,0x7bff,0x7bff,0x3c00};
    finite(render(TemporalSignal::DiffuseRadiance,outlier)[0]);
    reset();
    render(TemporalSignal::ReflectedRadiance,uniform({0x4800,0x4400,0x4000,0x3800}));
    const auto miss = render(TemporalSignal::ReflectedRadiance,uniform({0x7bff,0x7bff,0x7bff,0}));
    const auto missPixels = read(miss[0].signal.owner);
    require(std::abs(missPixels[0]-8) < 0.01f && std::abs(missPixels[1]-4) < 0.01f &&
        std::abs(missPixels[2]-2) < 0.01f && std::abs(missPixels[3]-0.25f) < 0.002f,
        "Zero-confidence miss RGB must not darken or contaminate retained hit radiance");
    const auto hit = render(TemporalSignal::ReflectedRadiance,uniform({0x4000,0x4400,0x4600,0x3800}));
    finite(hit[0]);
    require(std::abs(read(hit[0].signal.owner)[3]-1.0f/3) < 0.003f,
        "Reflection confidence must accumulate separately through miss-to-hit response");
    reset();
    for (uint32_t frame = 0; frame < 8; ++frame)
    {
        render(TemporalSignal::DiffuseRadiance,uniform({0x3c00,0,0,0x3c00}));
    }
    const auto chromatic = render(TemporalSignal::DiffuseRadiance,uniform({0,0,0x41e4,0x3c00}));
    const auto chromaticPixels = read(chromatic[0].signal.owner);
    require(chromaticPixels[0] > 0.5f && chromaticPixels[2] > 0 && chromaticPixels[2] < 1,
        "Similar-luminance red-to-blue change must record the known residual chromatic history limit");
    reset();
    if (metrics)
    {
        for (auto* accumulator : {&temporal,&baseline,control.get()})
        {
            accumulator->setMaxFrames(2);
            accumulator->setFreezeAtLimit(true);
        }
        render(TemporalSignal::Visibility,low);
        render(TemporalSignal::Visibility,low);
        const auto frozen = render(TemporalSignal::Visibility,high);
        const auto captured = read(frozen[0].metrics);
        require(captured[4*4] == 0 && captured[4*4+1] == 0 && captured[4*4+2] == 0 && captured[4*4+3] == 1,
            "Frozen diagnostics must report zero update/reactivity/unevaluated variance and saturated count");
        const auto thawed = render(TemporalSignal::Visibility,high,false,true);
        require(read(thawed[0].metrics)[4*4] > 0,"Motion thaw must report an active current weight");
        for (auto* accumulator : {&temporal,&baseline,control.get()})
        {
            accumulator->setFreezeAtLimit(false);
        }
    }
    temporal.setMaxFrames(1);
    baseline.setMaxFrames(1);
    if (control)
    {
        control->setMaxFrames(1);
    }
    render(TemporalSignal::Visibility,low);
    const auto one = render(TemporalSignal::Visibility,high);
    require(read(one[0].signal.owner)[4] == 0.75f && read(one[0].normal)[4*4+3] == 1,
        "maxFrames=1 must retain current samples and bounded count");
    temporal.cleanup();
    baseline.cleanup();
    if (control)
    {
        control->cleanup();
    }
    return true;
}
}
