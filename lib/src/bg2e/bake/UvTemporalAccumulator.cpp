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

#include <bg2e/bake/UvTemporalAccumulator.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include "OutputHelpers.hpp"
#include <cmath>
#include <utility>

namespace bg2e::bake {
UvTemporalAccumulator::UvTemporalAccumulator(AccumulationConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

UvTemporalAccumulator::~UvTemporalAccumulator()
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

void UvTemporalAccumulator::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || _configuration.shaderInterface != "bake.accumulation.v1")
    {
        throw std::invalid_argument("Bake accumulation requires clean instance, live Device/slots and compatible shaders");
    }
    _device = &device;
    for (auto format : {gpu::PixelFormat::R32G32B32A32_SFLOAT,gpu::PixelFormat::R32_UINT})
    {
        const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
        if (!device.formatCapabilities(format).supports(usage))
        {
            throw std::runtime_error("Required exact bake accumulation format/usage unavailable");
        }
    }
    _layout = device.createPipelineLayout(detail::outputLayout(6,2));
    gpu::ShaderLib library(_configuration.shaderRoot,backend);
    _shader = library.compute(_configuration.shaderPrefix+"/sum",&device);
    _pipeline = device.createComputePipeline({_shader.get(),_layout.get()});
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = gpu::Filter::Nearest;
    _nearest = device.createSampler(sampler);
    _slots.resize(slots);
}

std::shared_ptr<const AccumulationResult> UvTemporalAccumulator::record(gpu::CommandBuffer& command,
    uint32_t index, uint64_t serial, const BakeSample& sample,
    std::shared_ptr<const AccumulationResult> previous)
{
    if (!_device || !_pipeline || index >= _slots.size() || command.hasActiveScope())
    {
        throw std::logic_error("Accumulation requires an initialized retired slot outside command scopes");
    }
    const auto surface = sample.surface.surfaceInputs();
    surface.validate(*_device);
    const auto extent = surface.extent();
    const auto mode = sample.mode;
    if (mode != LightmapMode::RTAO && mode != LightmapMode::RTGI)
    {
        throw std::invalid_argument("Unknown raw bake signal mode");
    }
    detail::sampledImage(*_device,sample.signal,extent,mode == LightmapMode::RTAO ?
        gpu::PixelFormat::R16_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT);
    detail::sampledImage(*_device,sample.surface.images[3],extent,gpu::PixelFormat::R32_UINT);
    if ((!previous && sample.sampleIndex != 0) || sample.sampleIndex >= 1048576)
    {
        throw std::logic_error("Raw accumulation must start at zero and remain within exact sample bounds");
    }
    if (previous)
    {
        if (previous->mode != mode || previous->updates != sample.sampleIndex ||
            previous->surface.geometry != sample.surface.geometry ||
            previous->surface.sourceGeneration != sample.surface.sourceGeneration ||
            previous->surface.targetGeneration != sample.surface.targetGeneration)
        {
            throw std::logic_error("Accumulation requires the immediately preceding completed matching job");
        }
        detail::sampledImage(*_device,previous->sums,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
        detail::sampledImage(*_device,previous->counts,extent,gpu::PixelFormat::R32_UINT);
        detail::sampledImage(*_device,previous->surface.images[3],extent,gpu::PixelFormat::R32_UINT);
    }
    auto& slot = _slots[index];
    if (slot.used && serial <= slot.serial)
    {
        throw std::logic_error("Accumulation serial must advance on completed-slot reuse");
    }
    slot = {};
    slot.serial = serial;
    slot.used = true;
    slot.result = std::make_shared<AccumulationResult>(); // Retain before any command recording.
    auto& result = *slot.result;
    result.surface = sample.surface;
    result.mode = mode;
    result.updates = sample.sampleIndex+1;
    result.sums = detail::outputImage(*_device,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
    result.counts = detail::outputImage(*_device,extent,gpu::PixelFormat::R32_UINT);
    slot.inputs = {sample.signal,previous ? previous->sums : sample.signal,
        previous ? previous->counts : sample.surface.images[3],sample.surface.images[4],
        sample.surface.images[3],previous ? previous->surface.images[3] : sample.surface.images[3]};
    slot.uniforms = _device->createBuffer("Bake raw accumulation parameters");
    slot.uniforms->createUniformBuffer(glm::uvec4(uint32_t(mode),previous ? 1u : 0u,sample.sampleIndex,0));
    slot.set = _device->createResourceSet(_layout.get(),0);
    slot.set->setUniformBuffer({0,1},slot.uniforms.get());
    slot.set->setSampler({1,0},_nearest.get());
    for (uint32_t i = 0; i < slot.inputs.size(); ++i)
    {
        slot.set->setSampledImage({2+i,i},slot.inputs[i].get());
    }
    slot.set->setStorageImage({8,6},result.sums.get());
    slot.set->setStorageImage({9,7},result.counts.get());
    slot.set->update();
    detail::dispatch(command,*_pipeline,*slot.set,extent,slot.inputs,{result.sums,result.counts});
    return slot.result;
}

void UvTemporalAccumulator::validateCompleted(const AccumulationResult& result, uint32_t requested)
{
    if (!result.sums || !result.sums->device() || !result.sums->isValid() || !result.counts ||
        !requested || requested > 1048576 || requested != result.updates ||
        (result.mode != LightmapMode::RTAO && result.mode != LightmapMode::RTGI))
    {
        throw std::logic_error("Only the requested completed accumulation may be validated");
    }
    auto* device = result.sums->device();
    const auto extent = result.sums->size();
    detail::sampledImage(*device,result.sums,extent,gpu::PixelFormat::R32G32B32A32_SFLOAT);
    detail::sampledImage(*device,result.counts,extent,gpu::PixelFormat::R32_UINT);
    detail::sampledImage(*device,result.surface.images[3],extent,gpu::PixelFormat::R32_UINT);
    detail::sampledImage(*device,result.surface.images[4],extent,gpu::PixelFormat::R8_UNORM);
    const auto size = detail::pixels(extent);
    const auto sums = detail::read<float>(result.sums,size*4);
    const auto counts = detail::read<uint32_t>(result.counts,size);
    const auto charts = detail::read<uint32_t>(result.surface.images[3],size);
    const auto coverage = detail::read<uint8_t>(result.surface.images[4],size);
    size_t covered = 0;
    for (size_t i = 0; i < size; ++i)
    {
        const bool valid = charts[i] != 0;
        if ((coverage[i] != 0) != valid || (coverage[i] != 0 && coverage[i] != 255) ||
            counts[i] != (valid ? requested : 0u) || sums[i*4+3] != 0)
        {
            throw std::runtime_error("Bake has invalid/missing/saturated samples, changed coverage or numeric overflow");
        }
        for (size_t c = 0; c < 3; ++c)
        {
            const float value = sums[i*4+c];
            if (!std::isfinite(value) || value < 0 || (!valid && value != 0) ||
                (result.mode == LightmapMode::RTAO && value > float(requested)))
            {
                throw std::runtime_error("Bake raw sum is nonfinite, negative or outside its declared signal range");
            }
        }
        covered += valid ? 1u : 0u;
    }
    if (!covered)
    {
        throw std::runtime_error("Bake atlas contains no covered texels at the requested resolution");
    }
}

void UvTemporalAccumulator::cleanup()
{
    if (_device)
    {
        _device->waitIdle();
    }
    _slots.clear();
    _pipeline.reset();
    _shader.reset();
    _layout.reset();
    _nearest.reset();
    _device = nullptr;
}
}
