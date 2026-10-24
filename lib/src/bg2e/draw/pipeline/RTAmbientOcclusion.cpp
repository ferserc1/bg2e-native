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

#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) AOUniforms {
    glm::mat4 inverseViewProjection{1};
    glm::vec4 parameters{0}; // radius, bias, falloff, bounce attenuation
    glm::uvec4 counts{0}; // samples, bounces, instances, rendered-sample key
    glm::uvec4 noise{0}; // BlueNoise atlas metadata
    glm::uvec4 extent{0}; // surface width/height, output width/height
};
static_assert(sizeof(AOUniforms) == 128 && offsetof(AOUniforms, counts) == 80);
void positive(float value)
{
    if (!std::isfinite(value) || value <= 0)
    {
        throw std::invalid_argument("AO parameter must be finite and positive");
    }
}
}

gpu::Size2D SurfaceInputs::extent() const
{
    return normal ? normal->size() : gpu::Size2D{};
}

void SurfaceInputs::validate(gpu::Device& device) const
{
    if (profile != SurfaceProfile::CameraDepth && profile != SurfaceProfile::WorldPositionCoverage)
    {
        throw std::invalid_argument("Unknown effect surface profile");
    }
    for (auto* image : {normal,depthOrPosition})
    {
        if (!image || !image->isValid() || image->device() != &device || image->size().isZero() ||
            !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled) || image->imageType() != gpu::ImageType::Image2D ||
            image->size() != extent())
        {
            throw std::invalid_argument("Effect surface images require matching sampled same-device 2D extents");
        }
    }
    if (normal->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT)
    {
        throw std::invalid_argument("Effect surface normals require RGBA16 float");
    }
    if (profile == SurfaceProfile::CameraDepth)
    {
        if (depthOrPosition->pixelFormat() != gpu::PixelFormat::D32_SFLOAT && depthOrPosition->pixelFormat() != gpu::PixelFormat::D16_UNORM)
        {
            throw std::invalid_argument("Camera effect surface requires D16/D32 depth");
        }
        const auto determinant = glm::determinant(inverseViewProjection);
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f)
        {
            throw std::invalid_argument("Effect inverse view-projection must be invertible");
        }
        for (uint32_t c = 0; c < 4; ++c)
        {
            for (uint32_t r = 0; r < 4; ++r)
            {
                if (!std::isfinite(inverseViewProjection[c][r]))
                {
                    throw std::invalid_argument("Effect inverse view-projection must be finite");
                }
            }
        }
    }
    else if (depthOrPosition->pixelFormat() != gpu::PixelFormat::R32G32B32A32_SFLOAT || !coverage ||
        !coverage->isValid() || coverage->device() != &device || coverage->size() != extent() ||
        coverage->pixelFormat() != gpu::PixelFormat::R8_UNORM || !gpu::hasFlag(coverage->usage(),gpu::ImageUsage::Sampled) ||
        coverage->imageType() != gpu::ImageType::Image2D)
    {
        throw std::invalid_argument("Explicit effect surfaces require RGBA32 world positions and R8 coverage");
    }
}

gpu::PipelineLayoutDescription rtaoBindingLayout(uint32_t capacity, bool blueNoise)
{
    auto result = tracingBindingLayout(capacity);
    result.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
    for (uint32_t i = 0; i < 3; ++i)
    {
        result.resourceBindings.push_back({0,{1+i,i},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
    }
    result.resourceBindings.push_back({0,{4,9},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
    result.resourceBindings.push_back({0,{5,4},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
    if (blueNoise)
    {
        result.resourceBindings.push_back({0,{6,3},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
        result.resourceBindings.push_back({0,{7,10},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
    }
    return result;
}

RTAmbientOcclusion::RTAmbientOcclusion(RTAOConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void RTAmbientOcclusion::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || _configuration.shaderInterface != "draw.rtao.v3")
    {
        throw std::invalid_argument("AO initialization requires a clean instance, valid device/slots and draw.rtao.v3 shaders");
    }
    _device = &device;
    const auto usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    _capacity = device.resourceArrayCapabilities().profileCapacity(4,2,13);
    if (!device.rayTracingCapabilities().querySupported() || !_capacity ||
        !device.formatCapabilities(gpu::PixelFormat::R16_SFLOAT).supports(usage))
    {
        _capacity = 0;
        _status = "AO unavailable: compute queries, material arrays or scalar target support is missing";
        return;
    }
    if (_configuration.noise && &_configuration.noise->device() != &device)
    {
        throw std::invalid_argument("AO noise belongs to another device");
    }
    try
    {
        for (uint32_t index = 0; index < 4; ++index)
        {
            const bool world = index >= 2, blue = index % 2 != 0;
            _layouts[index] = device.createPipelineLayout(rtaoBindingLayout(_capacity,blue));
            gpu::ShaderLib library(_configuration.shaderRoot,backend);
            const auto name = _configuration.shaderPrefix+"/"+(world ? "world_" : "camera_")+
                (blue ? "blue_" : "hash_")+"256";
            // The filename is a deployed template, not the effective table size.
            _shaders[index] = backend == gpu::BackendType::Vulkan ?
                library.specialized(name,gpu::ShaderStage::Compute,&device,{{0,_capacity}}) :
                library.compute(name,&device);
            _pipelines[index] = device.createComputePipeline({_shaders[index].get(),_layouts[index].get()});
        }
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = device.createSampler(nearest);
        _tracing = std::make_shared<TracingSceneResources>(device,_capacity);
        _slots.resize(slots);
        _status = "AO compute queries ready";
    }
    catch (const std::exception& error)
    {
        _pipelines = {};
        _shaders = {};
        _layouts = {};
        _nearest.reset();
        _tracing.reset();
        _slots.clear();
        _capacity = 0;
        _status = std::string("AO unavailable: ")+error.what();
    }
}

void RTAmbientOcclusion::prepare(uint32_t slot, uint64_t serial, const RenderQueue& queue)
{
    if (available())
    {
        const auto& previous = _slots.at(slot);
        if (previous.prepared && previous.serial == serial)
        {
            throw std::logic_error("Prepare an AO slot once per acquired presentation/job sample");
        }
    }
    auto snapshot = available() && enabled() ? _tracing->prepare(queue) : nullptr;
    prepareSnapshot(slot,serial,std::move(snapshot));
}

void RTAmbientOcclusion::prepareSnapshot(uint32_t index, uint64_t serial, std::shared_ptr<TracingSnapshot> snapshot)
{
    if (!available())
    {
        return;
    }
    auto& slot = _slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Prepare an AO slot once per acquired presentation/job sample");
    }
    if (snapshot && (snapshot->device != _device || snapshot->capacity > _capacity))
    {
        throw std::invalid_argument("AO snapshot device/profile mismatch");
    }
    Slot next;
    next.prepared = true;
    next.serial = serial;
    next.snapshot = std::move(snapshot);
    slot = std::move(next); // Only the caller's retired slot may be replaced.
    if (_pool)
    {
        _pool->collect();
    }
}

void RTAmbientOcclusion::record(const RTAOContext& context)
{
    base::CpuProfileScope profile("effect.ao.record");
    const auto usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    if (!_device || context.command.hasActiveScope() || context.output.device() != _device || !context.output.isValid() ||
        context.output.pixelFormat() != gpu::PixelFormat::R16_SFLOAT || context.output.size().isZero() ||
        context.output.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::Storage) ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::Sampled) ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::ColorAttachment) ||
        !_device->formatCapabilities(gpu::PixelFormat::R16_SFLOAT).supports(usage))
    {
        throw std::invalid_argument("AO output requires compatible same-device R16 scalar image and inactive scope");
    }
    if (!available())
    {
        RenderTarget::clear(context.command,context.output,{1,1,1,1});
        return;
    }
    auto& slot = _slots.at(context.slot);
    if (!slot.prepared || slot.serial != context.serial)
    {
        throw std::logic_error("Prepare the acquired AO slot before recording");
    }
    if (!enabled() || !slot.snapshot || slot.snapshot->empty())
    {
        RenderTarget::clear(context.command,context.output,{1,1,1,1});
        return;
    }
    context.surface.validate(*_device);
    if (context.output.size() != outputExtent(context.surface.extent()) || context.output.imageType() != gpu::ImageType::Image2D ||
        &context.output == context.surface.normal || &context.output == context.surface.depthOrPosition || &context.output == context.surface.coverage)
    {
        throw std::invalid_argument("AO output extent/profile aliases or differs from the declared surface quality");
    }
    const bool blue = _settings.useBlueNoise && _configuration.noise && _configuration.noise->isReady();
    const uint32_t index = (context.surface.profile == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(blue ? 1u : 0u);
    Invocation invocation;
    AOUniforms data;
    data.inverseViewProjection = context.surface.inverseViewProjection;
    data.parameters = {_settings.radius,_settings.bias,_settings.falloff,_settings.bounceAttenuation};
    if (_settings.radius <= _settings.bias*2)
    {
        throw std::invalid_argument("AO radius must exceed twice its ray bias");
    }
    data.counts = {_settings.sampleCount,_settings.bounceCount,uint32_t(slot.snapshot->instances.size()),context.renderedSample};
    data.extent = {context.surface.extent().width,context.surface.extent().height,context.output.width(),context.output.height()};
    if (blue)
    {
        invocation.noiseImage = _configuration.noise->image();
        invocation.noiseSampler = _configuration.noise->sampler();
        data.noise = _configuration.noise->shaderMetadata();
        if (data.noise.z < 2)
        {
            throw std::invalid_argument("AO BlueNoise sampling requires two source channels");
        }
    }
    if (!_pool)
    {
        _pool = std::make_shared<TemporaryResourcePool>(*_device);
    }
    _pool->setEnabled(_poolingEnabled);
    invocation.pooled = _pool->bindings(context.command, _layouts[index], 0, sizeof(data),
        {context.slot, slot.invocations.size(), 0, 0});
    invocation.pooled->writeUniforms(&data, sizeof(data));
    invocation.uniforms = invocation.pooled->uniforms();
    slot.snapshot->build(context.command);
    invocation.sceneSet = slot.snapshot->resourceSet(*_layouts[index], 1, _capacity);
    invocation.tableSet = slot.snapshot->materialResourceSet(*_layouts[index], _capacity);
    invocation.surfaceSet = invocation.pooled->resources();
    invocation.pooled->setUniformBuffer({0,1},invocation.uniforms.get());
    invocation.pooled->setSampledImage({1,0},context.surface.normal);
    invocation.pooled->setSampledImage({2,1},context.surface.depthOrPosition);
    auto* coverage = context.surface.profile == SurfaceProfile::CameraDepth ? context.surface.normal : context.surface.coverage;
    invocation.pooled->setSampledImage({3,2},coverage);
    invocation.pooled->setSampler({4,9},_nearest.get());
    invocation.pooled->setStorageImage({5,4},&context.output);
    for (auto* image : {context.surface.normal,context.surface.depthOrPosition,coverage})
    {
        context.command.transition(image,gpu::ImageLayout::ShaderReadOnly);
    }
    if (blue)
    {
        invocation.pooled->setSampledImage({6,3},invocation.noiseImage.get());
        invocation.pooled->setSampler({7,10},invocation.noiseSampler.get());
        context.command.transition(invocation.noiseImage.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    invocation.pooled->update();
    slot.invocations.push_back(std::move(invocation));
    auto& retained = slot.invocations.back();
    context.command.transition(&context.output,gpu::ImageLayout::General);
    context.command.beginCompute();
    context.command.bindPipeline(_pipelines[index].get());
    context.command.bindResourceSet(_pipelines[index].get(),0,retained.surfaceSet.get());
    context.command.bindResourceSet(_pipelines[index].get(),1,retained.sceneSet.get());
    context.command.bindResourceSet(_pipelines[index].get(),2,retained.tableSet.get());
    context.command.dispatch((context.output.width()+15)/16,(context.output.height()+15)/16,1);
    context.command.endCompute();
    context.command.transition(&context.output,gpu::ImageLayout::ShaderReadOnly);
}

void RTAmbientOcclusion::reset()
{
    if (_device)
    {
        _device->waitIdleForResourceUpdate();
    }
    for (auto& slot : _slots)
    {
        slot = {};
    }
    if (_pool)
    {
        _pool->collect();
    }
    if (_tracing)
    {
        _tracing->clear();
    }
}

void RTAmbientOcclusion::cleanup()
{
    reset();
    _slots.clear();
    _pool.reset();
    _pipelines = {};
    _shaders = {};
    _layouts = {};
    _nearest.reset();
    _tracing.reset();
    _capacity = 0;
    _device = nullptr;
    _configuration.noise.reset();
    _status = "Not initialized";
}

const RTAOSettings& RTAmbientOcclusion::settings() const
{
    return _settings;
}

void RTAmbientOcclusion::setEnabled(bool value)
{
    _settings.enabled = value;
}

bool RTAmbientOcclusion::enabled() const
{
    return _settings.enabled;
}

void RTAmbientOcclusion::setQuality(RTAOQuality value)
{
    if (uint32_t(value) > uint32_t(RTAOQuality::Ultra))
    {
        throw std::invalid_argument("Unknown AO quality");
    }
    _settings.quality = value;
}

RTAOQuality RTAmbientOcclusion::quality() const
{
    return _settings.quality;
}

void RTAmbientOcclusion::setSampleCount(uint32_t value)
{
    if (!value || value > 64)
    {
        throw std::invalid_argument("AO sample count must be 1..64");
    }
    _settings.sampleCount = value;
}

uint32_t RTAmbientOcclusion::sampleCount() const
{
    return _settings.sampleCount;
}

void RTAmbientOcclusion::setBounceCount(uint32_t value)
{
    if (!value || value > 8)
    {
        throw std::invalid_argument("AO bounce count must be 1..8");
    }
    _settings.bounceCount = value;
}

uint32_t RTAmbientOcclusion::bounceCount() const
{
    return _settings.bounceCount;
}

void RTAmbientOcclusion::setRadius(float value)
{
    positive(value);
    _settings.radius = value;
}

float RTAmbientOcclusion::radius() const
{
    return _settings.radius;
}

void RTAmbientOcclusion::setBias(float value)
{
    positive(value);
    _settings.bias = value;
}

float RTAmbientOcclusion::bias() const
{
    return _settings.bias;
}

void RTAmbientOcclusion::setFalloff(float value)
{
    positive(value);
    _settings.falloff = value;
}

float RTAmbientOcclusion::falloff() const
{
    return _settings.falloff;
}

void RTAmbientOcclusion::setBounceAttenuation(float value)
{
    if (!std::isfinite(value) || value < 0 || value > 1)
    {
        throw std::invalid_argument("AO bounce attenuation must be in [0,1]");
    }
    _settings.bounceAttenuation = value;
}

float RTAmbientOcclusion::bounceAttenuation() const
{
    return _settings.bounceAttenuation;
}

void RTAmbientOcclusion::setUseBlueNoise(bool value)
{
    _settings.useBlueNoise = value;
}

bool RTAmbientOcclusion::useBlueNoise() const
{
    return _settings.useBlueNoise;
}

void RTAmbientOcclusion::setNoise(std::shared_ptr<utils::BlueNoise> noise)
{
    if (_device && noise && &noise->device() != _device)
    {
        throw std::invalid_argument("AO noise belongs to another device");
    }
    _configuration.noise = std::move(noise);
}

std::shared_ptr<utils::BlueNoise> RTAmbientOcclusion::noise() const
{
    return _configuration.noise;
}

bool RTAmbientOcclusion::available() const
{
    return bool(_pipelines[0]);
}

const std::string& RTAmbientOcclusion::status() const
{
    return _status;
}

uint32_t RTAmbientOcclusion::tableCapacity() const
{
    return _capacity;
}

std::shared_ptr<TracingSceneResources> RTAmbientOcclusion::tracingResources() const
{
    return _tracing;
}

gpu::Size2D RTAmbientOcclusion::outputExtent(gpu::Size2D surface) const
{
    if (surface.isZero())
    {
        throw std::invalid_argument("AO surface extent must be nonzero");
    }
    if (uint32_t(_settings.quality) > uint32_t(RTAOQuality::Ultra))
    {
        throw std::invalid_argument("Unknown AO quality");
    }
    const uint32_t numerator[] = {1,1,2,1}, denominator[] = {3,2,3,1};
    const auto n = numerator[uint32_t(_settings.quality)], d = denominator[uint32_t(_settings.quality)];
    return {std::max(1u,uint32_t((uint64_t(surface.width)*n+d/2)/d)),
            std::max(1u,uint32_t((uint64_t(surface.height)*n+d/2)/d))};
}

TemporaryPoolStatistics RTAmbientOcclusion::poolStatistics() const
{
    return _pool ? _pool->statistics() : TemporaryPoolStatistics{};
}

}
