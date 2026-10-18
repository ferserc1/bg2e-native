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

#include <bg2e/bake/LightmapCompositionPass.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include "OutputHelpers.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace bg2e::bake {
LightmapCompositionPass::LightmapCompositionPass(CompositionConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

LightmapCompositionPass::~LightmapCompositionPass()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
        // Explicit cleanup reports Device shutdown errors.
    }
}

void LightmapCompositionPass::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || _configuration.shaderInterface != "bake.composition.v1")
    {
        throw std::invalid_argument("Bake composition requires clean instance, live Device/slots and compatible shaders");
    }
    _device = &device;
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    for (auto format : {gpu::PixelFormat::R32G32B32A32_SFLOAT,gpu::PixelFormat::R32G32_UINT})
    {
        if (!device.formatCapabilities(format).supports(usage))
        {
            throw std::runtime_error("Required full-float bake composition/seed format unavailable");
        }
    }
    auto giLayout = detail::outputLayout(5,1);
    giLayout.resourceBindings.push_back({0,{8,1},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
    _layouts[0] = device.createPipelineLayout(detail::outputLayout(4,1));
    _layouts[1] = device.createPipelineLayout(giLayout);
    _layouts[2] = device.createPipelineLayout(detail::outputLayout(1,1));
    _layouts[3] = device.createPipelineLayout(detail::outputLayout(3,1));
    gpu::ShaderLib library(_configuration.shaderRoot,backend);
    const std::array<std::string,4> names{_configuration.shaderPrefix+"/ao",_configuration.shaderPrefix+"/gi",
        _configuration.dilationPrefix+"/seed",_configuration.dilationPrefix+"/apply"};
    for (size_t i = 0; i < names.size(); ++i)
    {
        _shaders[i] = library.compute(names[i],&device);
        _pipelines[i] = device.createComputePipeline({_shaders[i].get(),_layouts[i].get()});
    }
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = gpu::Filter::Nearest;
    _nearest = device.createSampler(sampler);
    _slots.resize(slots);
}

CompositionResult LightmapCompositionPass::record(gpu::CommandBuffer& command, uint32_t index, uint64_t serial,
    const AccumulationResult& accumulation, const LightmapSettings& settings,
    std::shared_ptr<const draw::environment::EnvironmentResources> environment)
{
    settings.validate();
    if (!_device || !_pipelines[3] || index >= _slots.size() || command.hasActiveScope() ||
        accumulation.mode != settings.mode() || accumulation.updates != settings.accumulationFrames())
    {
        throw std::logic_error("Bake composition requires final matching raw accumulation outside command scopes");
    }
    const gpu::Size2D extent{settings.resolution(),settings.resolution()};
    detail::sampledImage(*_device,accumulation.sums,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
    detail::sampledImage(*_device,accumulation.counts,extent,gpu::PixelFormat::R32_UINT);
    detail::sampledImage(*_device,accumulation.surface.images[3],extent,gpu::PixelFormat::R32_UINT);
    const auto surface = accumulation.surface.surfaceInputs();
    surface.validate(*_device);
    if (surface.extent() != extent)
    {
        throw std::invalid_argument("Composition surface extent differs from frozen target resolution");
    }
    const bool gi = settings.mode() == LightmapMode::RTGI;
    if (gi)
    {
        if (!environment)
        {
            throw std::invalid_argument("GI multiplier requires its retained reference environment");
        }
        environment->validate(_device);
    }
    auto& slot = _slots[index];
    if (slot.used && serial <= slot.serial)
    {
        throw std::logic_error("Composition serial must advance on completed-slot reuse");
    }
    slot = {};
    slot.serial = serial;
    slot.used = true;
    slot.environment = environment ? std::make_shared<draw::environment::EnvironmentResources>(*environment) : nullptr;
    const auto base = detail::outputImage(*_device,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
    slot.images.push_back(base);
    const auto seed = detail::outputImage(*_device,extent,gpu::PixelFormat::R32G32_UINT);
    slot.images.push_back(seed);
    const auto output = detail::outputImage(*_device,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
    slot.images.push_back(output);
    const auto charts = accumulation.surface.images[3];
    std::vector<std::shared_ptr<gpu::Image>> composeInputs{accumulation.sums,accumulation.counts,
        accumulation.surface.images[4],accumulation.surface.images[1]};
    if (gi)
    {
        composeInputs.push_back(slot.environment->irradiance);
    }
    // Input owners and every descriptor survive partial-record failures.
    slot.images.insert(slot.images.end(),composeInputs.begin(),composeInputs.end());
    slot.images.push_back(charts);
    slot.uniforms = _device->createBuffer("Bake composition and dilation parameters");
    slot.uniforms->createUniformBuffer(glm::uvec4(uint32_t(settings.mode()),settings.resolution(),
        settings.dilationPixels(),settings.accumulationFrames()));
    auto dispatch = [&](uint32_t pipeline, const std::vector<std::shared_ptr<gpu::Image>>& inputs,
        const std::shared_ptr<gpu::Image>& destination)
    {
        auto set = _device->createResourceSet(_layouts[pipeline].get(),0);
        slot.sets.push_back(set);
        set->setUniformBuffer({0,1},slot.uniforms.get());
        set->setSampler({1,0},_nearest.get());
        for (uint32_t i = 0; i < inputs.size(); ++i)
        {
            set->setSampledImage({2+i,i},inputs[i].get());
        }
        const auto n = uint32_t(inputs.size());
        set->setStorageImage({2+n,n},destination.get());
        if (pipeline == 1)
        {
            set->setSampler({8,1},slot.environment->sampler.get());
        }
        set->update();
        detail::dispatch(command,*_pipelines[pipeline],*set,extent,inputs,{destination});
    };
    dispatch(gi ? 1 : 0,composeInputs,base);
    dispatch(2,{charts},seed);
    dispatch(3,{base,charts,seed},output);
    return {output,accumulation.surface.sourceGeneration,accumulation.surface.targetGeneration,accumulation.mode};
}

LightmapPixels LightmapCompositionPass::convertRgba32(gpu::Size2D extent, const std::vector<float>& rgba,
    LightmapMode mode, LightmapPixelFormat format, float exposureEV)
{
    const auto count = detail::pixels(extent);
    if (rgba.size() != count*4 || (mode != LightmapMode::RTAO && mode != LightmapMode::RTGI) ||
        (format != LightmapPixelFormat::RGB8 && format != LightmapPixelFormat::RGB32F) ||
        !std::isfinite(exposureEV) || exposureEV < -32 || exposureEV > 32)
    {
        throw std::invalid_argument("Invalid completed RGBA32 bake payload/mode/format/exposure");
    }
    LightmapPixels pixels;
    pixels.width = extent.width;
    pixels.height = extent.height;
    pixels.format = format;
    if (format == LightmapPixelFormat::RGB32F)
    {
        pixels.rgb = std::vector<float>(count*3);
    }
    else
    {
        pixels.rgb = std::vector<uint8_t>(count*3);
    }
    const double exposure = mode == LightmapMode::RTGI ? std::exp2(double(exposureEV)) : 1.0;
    for (size_t i = 0; i < count; ++i)
    {
        if (rgba[i*4+3] != 1)
        {
            throw std::runtime_error("Completed bake image must have neutral alpha one");
        }
        for (size_t c = 0; c < 3; ++c)
        {
            const float value = rgba[i*4+c];
            if (!std::isfinite(value) || value < 0 || (mode == LightmapMode::RTAO && value > 1))
            {
                throw std::runtime_error("Bake multiplier contains negative/nonfinite values or exceeded actual GPU precision");
            }
            if (format == LightmapPixelFormat::RGB32F)
            {
                std::get<std::vector<float>>(pixels.rgb)[i*3+c] = value;
            }
            else
            {
                const double scaled = std::clamp(double(value)*exposure,0.0,1.0);
                std::get<std::vector<uint8_t>>(pixels.rgb)[i*3+c] = uint8_t(std::floor(scaled*255.0+0.5));
            }
        }
    }
    pixels.validate();
    return pixels;
}

LightmapPixels LightmapCompositionPass::readPixels(const CompositionResult& result, const LightmapSettings& settings)
{
    settings.validate();
    if (!result.image || !result.image->device() || !result.image->isValid() || result.mode != settings.mode())
    {
        throw std::logic_error("Missing/mismatched completed composition result");
    }
    const gpu::Size2D extent{settings.resolution(),settings.resolution()};
    detail::sampledImage(*result.image->device(),result.image,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
    return convertRgba32(extent,detail::read<float>(result.image,detail::pixels(extent)*4),
        result.mode,settings.cpuFormat(),settings.exposureEV());
}

void LightmapCompositionPass::cleanup()
{
    if (_device)
    {
        _device->waitIdle();
    }
    _slots.clear();
    for (auto& item : _pipelines)
    {
        item.reset();
    }
    for (auto& item : _shaders)
    {
        item.reset();
    }
    for (auto& item : _layouts)
    {
        item.reset();
    }
    _nearest.reset();
    _device = nullptr;
}
}
