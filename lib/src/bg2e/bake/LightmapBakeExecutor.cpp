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

#include <bg2e/bake/LightmapBakeExecutor.hpp>
#include <bg2e/gpu/Image.hpp>
#include <stdexcept>
#include <utility>

namespace bg2e::bake {
LightmapBakeExecutor::LightmapBakeExecutor(BakerContextConfiguration configuration)
    : _configuration(std::move(configuration))
{
    _configuration.validate();
    if (_configuration.environment)
    {
        _configuration.environment =
            std::make_shared<draw::environment::EnvironmentResources>(*_configuration.environment);
    }
}

LightmapBakeExecutor::~LightmapBakeExecutor()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
        // Explicit cleanup reports Device shutdown failures to the owner.
    }
}

void LightmapBakeExecutor::initialize(const LightmapSettings& settings)
{
    settings.validate();
    if (_initialized || _surface || _ao || _gi)
    {
        throw std::logic_error("Clean up the exclusive bake executor before reinitialization");
    }
    if (_configuration.passes.denoise)
    {
        throw std::invalid_argument("Bake filtering requires chart isolation; this executor records raw samples only");
    }
    _settings = settings;
    auto& device = *_configuration.device;
    if (settings.resolution() > device.renderingCapabilities().maxImageDimension2D)
    {
        throw std::length_error("Bake resolution exceeds Device image dimensions");
    }
    const bool gi = settings.mode() == LightmapMode::RTGI;
    const auto format = gi ? gpu::PixelFormat::R16G16B16A16_SFLOAT : gpu::PixelFormat::R16_SFLOAT;
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage |
        gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    if (!device.formatCapabilities(format).supports(usage))
    {
        throw std::runtime_error("Required raw bake signal format/usage is unsupported");
    }
    if (gi)
    {
        if (!_configuration.environment)
        {
            throw std::invalid_argument("GI bake requires retained explicit environment resources");
        }
        _configuration.environment->validate(&device);
    }
    try
    {
        UvSurfaceConfiguration surfaceConfiguration;
        surfaceConfiguration.shaderRoot = _configuration.shaderRoot;
        _surface = _configuration.passes.surface ? _configuration.passes.surface :
            std::make_shared<UvSurfacePass>(surfaceConfiguration);
        _surface->initialize(device,_configuration.backend,_configuration.frameSlots);
        if (gi)
        {
            draw::pipeline::RTGIConfiguration configuration;
            configuration.shaderRoot = _configuration.shaderRoot;
            _gi = _configuration.passes.globalIllumination ? _configuration.passes.globalIllumination :
                std::make_shared<draw::pipeline::RTGlobalIllumination>(configuration);
            _gi->initialize(device,_configuration.backend,_configuration.frameSlots);
            if (!_gi->available())
            {
                throw std::runtime_error("Required GI bake profile unavailable: "+_gi->status());
            }
            _gi->setEnabled(true);
            _gi->setQuality(draw::pipeline::RTGIQuality::Ultra);
            _gi->setSampleCount(settings.samplesPerPixel());
            _gi->setBounceCount(settings.giBounces());
            _gi->setMaxDistance(settings.maxRayDistance());
            _gi->setBias(settings.rayBias());
        }
        else
        {
            draw::pipeline::RTAOConfiguration configuration;
            configuration.shaderRoot = _configuration.shaderRoot;
            _ao = _configuration.passes.ambientOcclusion ? _configuration.passes.ambientOcclusion :
                std::make_shared<draw::pipeline::RTAmbientOcclusion>(configuration);
            _ao->initialize(device,_configuration.backend,_configuration.frameSlots);
            if (!_ao->available())
            {
                throw std::runtime_error("Required AO bake profile unavailable: "+_ao->status());
            }
            _ao->setEnabled(true);
            _ao->setQuality(draw::pipeline::RTAOQuality::Ultra);
            _ao->setSampleCount(settings.samplesPerPixel());
            _ao->setBounceCount(1);
            _ao->setRadius(settings.aoRadius());
            _ao->setBias(settings.rayBias());
        }
        _slots.resize(_configuration.frameSlots);
        _initialized = true;
    }
    catch (...)
    {
        cleanup();
        throw;
    }
}

void LightmapBakeExecutor::cleanup()
{
    if (_surface || _ao || _gi)
    {
        _configuration.device->waitIdle();
    }
    if (_ao)
    {
        _ao->cleanup();
    }
    if (_gi)
    {
        _gi->cleanup();
    }
    if (_surface)
    {
        _surface->cleanup();
    }
    _slots.clear();
    _surface.reset();
    _ao.reset();
    _gi.reset();
    _initialized = false;
}

std::shared_ptr<const UvSurfaceGeometry> LightmapBakeExecutor::prepareGeometry(std::shared_ptr<draw::Drawable> drawable)
{
    if (!_initialized)
    {
        throw std::logic_error("Initialize bake executor before geometry preparation");
    }
    return _surface->prepare(std::move(drawable));
}

std::shared_ptr<draw::pipeline::TracingSnapshot> LightmapBakeExecutor::prepareFrame(
    uint32_t slot, uint64_t serial, const draw::RenderQueue& queue)
{
    if (!_initialized || slot >= _slots.size())
    {
        throw std::invalid_argument("Invalid bake executor slot/lifecycle");
    }
    if (_slots[slot].prepared && serial <= _slots[slot].serial)
    {
        throw std::logic_error("Bake executor serial must advance on each completed-slot reuse");
    }
    auto resources = _gi ? _gi->tracingResources() : _ao->tracingResources();
    if (!resources)
    {
        throw std::runtime_error("Required bake tracing resource builder unavailable");
    }
    auto snapshot = resources->prepare(queue);
    if (_gi && snapshot->empty())
    {
        throw std::runtime_error("GI bake rejects an empty traceable scene; environment-only policy is unsupported");
    }
    _surface->beginFrame(slot,serial);
    if (_gi)
    {
        _gi->prepareSnapshot(slot,serial,snapshot);
    }
    else
    {
        _ao->prepareSnapshot(slot,serial,snapshot);
    }
    _slots[slot] = {serial,true,false,std::move(snapshot),nullptr};
    return _slots[slot].snapshot;
}

BakeSample LightmapBakeExecutor::record(const UvSurfaceInput& input, uint32_t sampleIndex,
    std::shared_ptr<draw::pipeline::TracingSnapshot> snapshot, const draw::LightUniforms& lights)
{
    if (!_initialized || input.slot >= _slots.size() || input.command.hasActiveScope())
    {
        throw std::invalid_argument("Bake sample requires initialized executor, valid slot and inactive command scope");
    }
    auto& slot = _slots[input.slot];
    const gpu::Size2D extent{_settings.resolution(),_settings.resolution()};
    if (!slot.prepared || slot.recorded || slot.serial != input.serial || snapshot != slot.snapshot ||
        !snapshot || input.extent != extent || sampleIndex >= _settings.accumulationFrames())
    {
        throw std::logic_error("Bake sample must match prepared snapshot, extent, serial and sample range exactly once");
    }
    const auto outputExtent = _gi ? _gi->outputExtent(extent) : _ao->outputExtent(extent);
    const bool settingsChanged = _gi ?
        (_gi->sampleCount() != _settings.samplesPerPixel() || _gi->bounceCount() != _settings.giBounces() ||
            _gi->bias() != _settings.rayBias() || _gi->maxDistance() != _settings.maxRayDistance()) :
        (_ao->sampleCount() != _settings.samplesPerPixel() || _ao->bounceCount() != 1 ||
            _ao->bias() != _settings.rayBias() || _ao->radius() != _settings.aoRadius());
    if (outputExtent != extent || settingsChanged || (_gi && !_gi->enabled()) || (_ao && !_ao->enabled()))
    {
        throw std::logic_error("Exclusive bake effect settings must remain frozen, enabled and full resolution");
    }
    slot.recorded = true; // A failed/partial command may never be retried.
    slot.sample = std::make_shared<BakeSample>();
    auto& sample = *slot.sample;
    sample.snapshot = std::move(snapshot);
    sample.environment = _configuration.environment;
    sample.lights = lights;
    sample.sampleIndex = sampleIndex;
    sample.mode = _settings.mode();
    gpu::ImageDescription description;
    description.size = extent;
    description.format = _gi ? gpu::PixelFormat::R16G16B16A16_SFLOAT : gpu::PixelFormat::R16_SFLOAT;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage |
        gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    sample.signal = _configuration.device->createImage(description);
    sample.surface = _surface->record(input);
    if (sample.surface.geometry != input.geometry || sample.surface.sourceGeneration != input.sourceGeneration ||
        sample.surface.targetGeneration != input.targetGeneration)
    {
        throw std::logic_error("UV surface replacement returned mismatched geometry/generation owners");
    }
    if (_gi)
    {
        _gi->record({input.command,input.slot,input.serial,sampleIndex,sample.surface.surfaceInputs(),
            *sample.signal,lights,sample.environment});
    }
    else
    {
        _ao->record({input.command,input.slot,input.serial,sampleIndex,sample.surface.surfaceInputs(),*sample.signal});
    }
    return sample;
}

const LightmapSettings& LightmapBakeExecutor::settings() const
{
    return _settings;
}
}
