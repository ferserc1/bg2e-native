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

#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) GIUniforms {
    glm::mat4 inverseViewProjection{1};
    glm::vec4 parameters{0}; // max distance, bias, shadow samples, shadows enabled
    glm::uvec4 counts{0}; // samples, bounces, instances, rendered-sample key
    glm::uvec4 noise{0}; // BlueNoise atlas metadata
    glm::uvec4 extent{0}; // surface width/height, output width/height
    LightUniforms lights;
};
static_assert(sizeof(GIUniforms) == 784 && offsetof(GIUniforms, counts) == 80 && offsetof(GIUniforms, lights) == 128);
void positive(float value)
{
    if (!std::isfinite(value) || value <= 0)
    {
        throw std::invalid_argument("GI parameter must be finite and positive");
    }
}
}

gpu::PipelineLayoutDescription rtgiBindingLayout(uint32_t capacity, bool blueNoise, gpu::BackendType backend)
{
    const auto stage = backend == gpu::BackendType::Vulkan ? gpu::ShaderStage::RayGeneration : gpu::ShaderStage::Compute;
    auto result = tracingBindingLayout(capacity,stage,backend == gpu::BackendType::Vulkan ?
        std::vector<gpu::ShaderStage>{gpu::ShaderStage::AnyHit} : std::vector<gpu::ShaderStage>{});
    result.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,stage});
    for (uint32_t i = 0; i < 3; ++i)
    {
        result.resourceBindings.push_back({0,{1+i,i},gpu::ResourceType::SampledImage,stage});
    }
    result.resourceBindings.push_back({0,{4,9},gpu::ResourceType::Sampler,stage});
    result.resourceBindings.push_back({0,{5,4},gpu::ResourceType::StorageImage,stage});
    if (blueNoise)
    {
        result.resourceBindings.push_back({0,{6,3},gpu::ResourceType::SampledImage,stage});
        result.resourceBindings.push_back({0,{7,10},gpu::ResourceType::Sampler,stage});
    }
    result.resourceBindings.push_back({0,{8,5},gpu::ResourceType::SampledImage,stage});
    result.resourceBindings.push_back({0,{9,11},gpu::ResourceType::Sampler,stage});
    return result;
}

RTGlobalIllumination::RTGlobalIllumination(RTGIConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void RTGlobalIllumination::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || _configuration.shaderInterface != "draw.rtgi.v1")
    {
        throw std::invalid_argument("GI initialization requires a clean instance, valid device/slots and draw.rtgi.v1 shaders");
    }
    _device = &device;
    _backend = backend;
    const auto usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    _capacity = device.resourceArrayCapabilities().profileCapacity(5,3,15);
    if (!(backend == gpu::BackendType::Vulkan ? device.rayTracingCapabilities().pipelineSupported() : device.rayTracingCapabilities().querySupported()) || !_capacity ||
        !device.formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(usage))
    {
        _capacity = 0;
        _status = "GI unavailable: native tracing, material arrays or HDR target support is missing";
        return;
    }
    if (_configuration.noise && &_configuration.noise->device() != &device)
    {
        throw std::invalid_argument("GI noise belongs to another device");
    }
    try
    {
        for (uint32_t index = 0; index < 4; ++index)
        {
            const bool world = index >= 2, blue = index % 2 != 0;
            _layouts[index] = device.createPipelineLayout(rtgiBindingLayout(_capacity,blue,backend));
            gpu::ShaderLib library(_configuration.shaderRoot,backend);
            const auto name = _configuration.shaderPrefix+"/"+(world ? "world_" : "camera_")+
                (blue ? "blue_" : "hash_")+std::to_string(_capacity);
            if (backend == gpu::BackendType::Vulkan)
            {
                _shaders[index] = library.rayGeneration(name,&device);
                _stages[index*3] = library.miss(name,&device);
                _stages[index*3+1] = library.closestHit(name,&device);
                _stages[index*3+2] = library.anyHit(name,&device);
                gpu::RayTracingPipelineDescription description;
                description.raygenShader = _shaders[index].get();
                description.missShader = _stages[index*3].get();
                description.closestHitShader = _stages[index*3+1].get();
                description.anyHitShader = _stages[index*3+2].get();
                description.layout = _layouts[index].get();
                description.maxRecursionDepth = 1;
                _pipelines[index] = device.createRayTracingPipeline(description);
            }
            else
            {
                _shaders[index] = library.compute(name,&device);
                _compute[index] = device.createComputePipeline({_shaders[index].get(),_layouts[index].get()});
            }
        }
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = device.createSampler(nearest);
        _tracing = std::make_shared<TracingSceneResources>(device,_capacity);
        _slots.resize(slots);
        _status = "GI native transport ready";
    }
    catch (const std::exception& error)
    {
        _pipelines = {};
        _compute = {};
        _stages = {};
        _shaders = {};
        _layouts = {};
        _nearest.reset();
        _tracing.reset();
        _slots.clear();
        _capacity = 0;
        _status = std::string("GI unavailable: ")+error.what();
    }
}

void RTGlobalIllumination::prepare(uint32_t slot, uint64_t serial, const RenderQueue& queue)
{
    if (available())
    {
        const auto& previous = _slots.at(slot);
        if (previous.prepared && previous.serial == serial)
        {
            throw std::logic_error("Prepare an GI slot once per acquired presentation/job sample");
        }
    }
    auto snapshot = available() && enabled() ? _tracing->prepare(queue) : nullptr;
    prepareSnapshot(slot,serial,std::move(snapshot));
}

void RTGlobalIllumination::prepareSnapshot(uint32_t index, uint64_t serial, std::shared_ptr<TracingSnapshot> snapshot)
{
    if (!available())
    {
        return;
    }
    auto& slot = _slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Prepare an GI slot once per acquired presentation/job sample");
    }
    if (snapshot && (snapshot->device != _device || snapshot->capacity != _capacity))
    {
        throw std::invalid_argument("GI snapshot device/profile mismatch");
    }
    Slot next;
    next.prepared = true;
    next.serial = serial;
    next.snapshot = std::move(snapshot);
    slot = std::move(next); // Only the caller's retired slot may be replaced.
}

void RTGlobalIllumination::record(const RTGIContext& context)
{
    const auto usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    if (!_device || context.command.hasActiveScope() || context.output.device() != _device || !context.output.isValid() ||
        context.output.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT || context.output.size().isZero() ||
        context.output.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::Storage) ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::Sampled) ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::ColorAttachment) ||
        !_device->formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(usage))
    {
        throw std::invalid_argument("GI output requires compatible same-device RGBA16 HDR image and inactive scope");
    }
    if (!available())
    {
        RenderTarget::clear(context.command,context.output,{0,0,0,0});
        return;
    }
    auto& slot = _slots.at(context.slot);
    if (!slot.prepared || slot.serial != context.serial)
    {
        throw std::logic_error("Prepare the acquired GI slot before recording");
    }
    if (!enabled() || !slot.snapshot || slot.snapshot->empty())
    {
        RenderTarget::clear(context.command,context.output,{0,0,0,0});
        return;
    }
    context.surface.validate(*_device);
    if (context.output.size() != outputExtent(context.surface.extent()) || context.output.imageType() != gpu::ImageType::Image2D ||
        &context.output == context.surface.normal || &context.output == context.surface.depthOrPosition || &context.output == context.surface.coverage)
    {
        throw std::invalid_argument("GI output extent/profile aliases or differs from the declared surface quality");
    }
    const bool blue = _settings.useBlueNoise && _configuration.noise && _configuration.noise->isReady();
    const uint32_t index = (context.surface.profile == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(blue ? 1u : 0u);
    Invocation invocation;
    GIUniforms data;
    data.inverseViewProjection = context.surface.inverseViewProjection;
    data.parameters = {_settings.maxDistance,_settings.bias,float(_settings.shadowSamples),_settings.useShadows ? 1.0f : 0.0f};
    if (_settings.maxDistance <= _settings.bias*2)
    {
        throw std::invalid_argument("GI maximum distance must exceed twice its ray bias");
    }
    if (!context.environment)
    {
        throw std::invalid_argument("GI requires retained environment resources");
    }
    context.environment->validate(_device);
    if (context.lights.count.x > MaxLights)
    {
        throw std::invalid_argument("GI light count exceeds the shared light ABI");
    }
    for (uint32_t i = 0; i < context.lights.count.x; ++i)
    {
        const auto& light = context.lights.lights[i];
        for (auto vector : {light.positionIntensity,light.color,light.directionAngle,light.parameters})
        {
            for (uint32_t c = 0; c < 4; ++c)
            {
                if (!std::isfinite(vector[c]))
                {
                    throw std::invalid_argument("GI light values must be finite");
                }
            }
        }
    }
    invocation.environment = context.environment;
    data.lights = context.lights;
    data.counts = {_settings.sampleCount,_settings.bounceCount,uint32_t(slot.snapshot->instances.size()),context.renderedSample};
    data.extent = {context.surface.extent().width,context.surface.extent().height,context.output.width(),context.output.height()};
    if (blue)
    {
        invocation.noiseImage = _configuration.noise->image();
        invocation.noiseSampler = _configuration.noise->sampler();
        data.noise = _configuration.noise->shaderMetadata();
        if (data.noise.z < 2)
        {
            throw std::invalid_argument("GI BlueNoise sampling requires two source channels");
        }
    }
    invocation.uniforms = _device->createBuffer("GI invocation uniforms");
    invocation.uniforms->createUniformBuffer(data);
    slot.snapshot->build(context.command);
    invocation.sceneSet = slot.snapshot->resourceSet(*_layouts[index]);
    invocation.surfaceSet = _device->createResourceSet(_layouts[index].get(),0);
    invocation.surfaceSet->setUniformBuffer({0,1},invocation.uniforms.get());
    invocation.surfaceSet->setSampledImage({1,0},context.surface.normal);
    invocation.surfaceSet->setSampledImage({2,1},context.surface.depthOrPosition);
    auto* coverage = context.surface.profile == SurfaceProfile::CameraDepth ? context.surface.normal : context.surface.coverage;
    invocation.surfaceSet->setSampledImage({3,2},coverage);
    invocation.surfaceSet->setSampler({4,9},_nearest.get());
    invocation.surfaceSet->setStorageImage({5,4},&context.output);
    for (auto* image : {context.surface.normal,context.surface.depthOrPosition,coverage})
    {
        context.command.transition(image,gpu::ImageLayout::ShaderReadOnly);
    }
    if (blue)
    {
        invocation.surfaceSet->setSampledImage({6,3},invocation.noiseImage.get());
        invocation.surfaceSet->setSampler({7,10},invocation.noiseSampler.get());
        context.command.transition(invocation.noiseImage.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    invocation.surfaceSet->setSampledImage({8,5},invocation.environment->irradiance.get());
    invocation.surfaceSet->setSampler({9,11},invocation.environment->sampler.get());
    context.command.transition(invocation.environment->irradiance.get(),gpu::ImageLayout::ShaderReadOnly);
    invocation.surfaceSet->update();
    slot.invocations.push_back(std::move(invocation));
    auto& retained = slot.invocations.back();
    context.command.transition(&context.output,gpu::ImageLayout::General);
    if (_backend == gpu::BackendType::Vulkan)
    {
        context.command.bindPipeline(_pipelines[index].get());
        context.command.bindResourceSet(_pipelines[index].get(),0,retained.surfaceSet.get());
        context.command.bindResourceSet(_pipelines[index].get(),1,retained.sceneSet.get());
        context.command.traceRays(context.output.width(),context.output.height(),1);
    }
    else
    {
        context.command.beginCompute();
        context.command.bindPipeline(_compute[index].get());
        context.command.bindResourceSet(_compute[index].get(),0,retained.surfaceSet.get());
        context.command.bindResourceSet(_compute[index].get(),1,retained.sceneSet.get());
        context.command.dispatch((context.output.width()+15)/16,(context.output.height()+15)/16,1);
        context.command.endCompute();
    }
    context.command.transition(&context.output,gpu::ImageLayout::ShaderReadOnly);
}

void RTGlobalIllumination::reset()
{
    if (_device)
    {
        _device->waitIdle();
    }
    for (auto& slot : _slots)
    {
        slot = {};
    }
    if (_tracing)
    {
        _tracing->clear();
    }
}

void RTGlobalIllumination::cleanup()
{
    reset();
    _slots.clear();
    _pipelines = {};
    _compute = {};
    _stages = {};
    _shaders = {};
    _layouts = {};
    _nearest.reset();
    _tracing.reset();
    _capacity = 0;
    _device = nullptr;
    _configuration.noise.reset();
    _status = "Not initialized";
}

const RTGISettings& RTGlobalIllumination::settings() const
{
    return _settings;
}

void RTGlobalIllumination::setEnabled(bool value)
{
    _settings.enabled = value;
}

bool RTGlobalIllumination::enabled() const
{
    return _settings.enabled;
}

void RTGlobalIllumination::setQuality(RTGIQuality value)
{
    if (uint32_t(value) > uint32_t(RTGIQuality::Ultra))
    {
        throw std::invalid_argument("Unknown GI quality");
    }
    _settings.quality = value;
}

RTGIQuality RTGlobalIllumination::quality() const
{
    return _settings.quality;
}

void RTGlobalIllumination::setSampleCount(uint32_t value)
{
    if (!value || value > 64)
    {
        throw std::invalid_argument("GI sample count must be 1..64");
    }
    _settings.sampleCount = value;
}

uint32_t RTGlobalIllumination::sampleCount() const
{
    return _settings.sampleCount;
}

void RTGlobalIllumination::setBounceCount(uint32_t value)
{
    if (!value || value > 8)
    {
        throw std::invalid_argument("GI bounce count must be 1..8");
    }
    _settings.bounceCount = value;
}

uint32_t RTGlobalIllumination::bounceCount() const
{
    return _settings.bounceCount;
}

void RTGlobalIllumination::setBias(float value)
{
    positive(value);
    _settings.bias = value;
}

float RTGlobalIllumination::bias() const
{
    return _settings.bias;
}

void RTGlobalIllumination::setUseBlueNoise(bool value)
{
    _settings.useBlueNoise = value;
}

bool RTGlobalIllumination::useBlueNoise() const
{
    return _settings.useBlueNoise;
}

void RTGlobalIllumination::setNoise(std::shared_ptr<utils::BlueNoise> noise)
{
    if (_device && noise && &noise->device() != _device)
    {
        throw std::invalid_argument("GI noise belongs to another device");
    }
    _configuration.noise = std::move(noise);
}

std::shared_ptr<utils::BlueNoise> RTGlobalIllumination::noise() const
{
    return _configuration.noise;
}

bool RTGlobalIllumination::available() const
{
    return bool(_pipelines[0]) || bool(_compute[0]);
}

const std::string& RTGlobalIllumination::status() const
{
    return _status;
}

uint32_t RTGlobalIllumination::tableCapacity() const
{
    return _capacity;
}

std::shared_ptr<TracingSceneResources> RTGlobalIllumination::tracingResources() const
{
    return _tracing;
}

gpu::Size2D RTGlobalIllumination::outputExtent(gpu::Size2D surface) const
{
    if (surface.isZero())
    {
        throw std::invalid_argument("GI surface extent must be nonzero");
    }
    if (uint32_t(_settings.quality) > uint32_t(RTGIQuality::Ultra))
    {
        throw std::invalid_argument("Unknown GI quality");
    }
    const uint32_t numerator[] = {1,1,2,1}, denominator[] = {3,2,3,1};
    const auto n = numerator[uint32_t(_settings.quality)], d = denominator[uint32_t(_settings.quality)];
    return {std::max(1u,uint32_t((uint64_t(surface.width)*n+d/2)/d)),
            std::max(1u,uint32_t((uint64_t(surface.height)*n+d/2)/d))};
}

void RTGlobalIllumination::setMaxDistance(float value)
{
    positive(value);
    _settings.maxDistance = value;
}

float RTGlobalIllumination::maxDistance() const
{
    return _settings.maxDistance;
}

void RTGlobalIllumination::setShadowSamples(uint32_t value)
{
    if (!value || value > 64)
    {
        throw std::invalid_argument("GI shadow samples must be 1..64");
    }
    _settings.shadowSamples = value;
}

uint32_t RTGlobalIllumination::shadowSamples() const
{
    return _settings.shadowSamples;
}

void RTGlobalIllumination::setUseShadows(bool value)
{
    _settings.useShadows = value;
}

bool RTGlobalIllumination::useShadows() const
{
    return _settings.useShadows;
}
}
